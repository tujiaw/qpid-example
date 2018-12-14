#include "ConnectionService.h"
#include "qpid/messaging/Connection.h"
#include "qpid/messaging/Address.h"

#ifdef WIN32
    #pragma comment(lib, "rpcrt4.lib")  // UuidCreate - Minimum supported OS Win 2000
    #include <windows.h>
    #include <iostream>
#else
    // uuid
    #include <boost/uuid/uuid.hpp>
    #include <boost/uuid/uuid_generators.hpp>
    #include <boost/uuid/uuid_io.hpp>
    #include <boost/locale/encoding.hpp>
    #include <boost/date_time/posix_time/posix_time.hpp>  
    #include <boost/thread/mutex.hpp>
#endif

time_t GetCurrentTimeSec()
{
    return time(NULL);
}

std::string NewMessageId()
{
#ifdef WIN32
    UUID uuid;
    UuidCreate(&uuid);
    char *str;
    UuidToStringA(&uuid, (RPC_CSTR*)&str);
    std::string result(str);
    RpcStringFreeA((RPC_CSTR*)&str);
    return result;
#else
    static boost::uuids::random_generator rg;
    static boost::mutex rg_mutex;
    boost::mutex::scoped_lock lock(rg_mutex);
    boost::uuids::uuid u = rg();
    std::string result = boost::lexical_cast<std::string>(u);
    return result;
#endif
}

ConnectionService::ConnectionService(const std::string &url, const std::string &sendAddr)
    : _connection(new Connection(url))
    , _is_open(false)
{
    _connection->setOption("reconnect", true);
    _connection->setOption("heartbeat", 5);
    _requestAddr = sendAddr + "; {create:always, node:{type:queue}}";
    Open(url);
}

ConnectionService::~ConnectionService()
{
    Close();
}

void ConnectionService::AddHandler(const std::string &serverAddr, const ServerCallback &cb)
{
    if (!serverAddr.empty() && cb) {
        std::string serverAddress = serverAddr + "; {create:always, node : {type:queue}}";
        std::shared_ptr<std::thread> t(new std::thread(std::bind(&ConnectionService::HandlerRunning, this, serverAddress, cb)));
        _handlerThreadList.push_back(t);
    }
}

bool ConnectionService::Open(const std::string& url)
{
    std::cout << "ConnectionService::Open url:" << url;
    try {
        _connection->open();
        _session = _connection->createSession();
        _sender = _session.createSender(_requestAddr);
        _asyncReceiver = _session.createReceiver("#.ConnectionService");
        _syncReceiver = _session.createReceiver("#.ConnectionService");
    } catch (const std::exception& error) {
        std::cerr << "ConnectionService::Open error:" << error.what();
    }

    _is_open = _connection->isOpen();
    if (_is_open) {
        StartThread();
    }
    return _is_open;
}

void ConnectionService::Close()
{
    _is_open = false;

    if (_receiveThread && _receiveThread->joinable()) {
        _receiveThread->join();
    }

    for (const auto &t : _handlerThreadList) {
        if (t->joinable()) {
            t->join();
        }
    }

    try {
        _sender.close();
        _asyncReceiver.close();
        _syncReceiver.close();
        _sender.close();
    } catch (const std::exception& error) {
        std::cerr << "ConnectionService::Close, error:" << error.what();
    }

    try {
        _connection->close();
    } catch (const std::exception &error) {
        std::cerr << "ConnectionService::Close, connecton close error:" << error.what();
    }
}

void ConnectionService::StartThread()
{
    if (!_receiveThread) {
        _receiveThread.reset(new std::thread(std::bind(&ConnectionService::ReceiveRunning, this)));
    }

    std::thread timeoutThread(std::bind(&ConnectionService::TimeoutRunning, this));
    timeoutThread.detach();
}

void ConnectionService::ReceiveRunning()
{
    while (_is_open) {
        try {
            QMsgPtr p(new Message());
            while (_asyncReceiver.fetch(*p.get(), Duration::SECOND)) {
                RequestInfo info;
                {
                    std::unique_lock<std::mutex> lock(_requestCacheMutex);
                    auto iter = _requestCache.find(p->getMessageId());
                    if (iter != _requestCache.end()) {
                        info = iter->second;
                        _requestCache.erase(iter);
                    }
                }
                if (info.cb && info.msg) {
                    info.cb(info.msg, p);
                }
                _session.acknowledge();
            }
            _session.sync();
        } catch (const std::exception& error) {
            std::cerr << "ConnectionService::ReceiveRunning error:" << error.what();
        }
    }
}

void ConnectionService::TimeoutRunning()
{
    while (_is_open) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (_requestCache.empty()) {
            continue;
        }

        std::vector<RequestInfo> timeoutList;
        {
            time_t currentTime = GetCurrentTimeSec();
            std::unique_lock<std::mutex> lock(_requestCacheMutex);
            auto iter = _requestCache.begin();
            while (iter != _requestCache.end()) {
                const RequestInfo &info = iter->second;
                if (currentTime - info.time >= info.second) {
                    timeoutList.push_back(info);
                    iter = _requestCache.erase(iter);
                } else {
                    ++iter; 
                }
            }
        }

        if (!timeoutList.empty()) {
            for (auto iter = timeoutList.begin(); iter != timeoutList.end(); ++iter) {
                iter->cb(iter->msg, QMsgPtr(new Message()));
            }
        }
    }
}

void ConnectionService::HandlerRunning(const std::string &addr, const ServerCallback &cb)
{
    while (_is_open) {
        Session session = _connection->createSession();
        Receiver receiver = session.createReceiver(addr);

        try {
            while (_is_open) {
                Message msg;
                if (receiver.fetch(msg, Duration::SECOND)) {
                    Message reply;
                    cb(msg, reply);
                    session.acknowledge();
                    const Address &address = msg.getReplyTo();
                    const std::string &msgid = msg.getMessageId();
                    if (address && !msgid.empty()) {
                        reply.setMessageId(msgid);
                        Sender sender = session.createSender(address);
                        sender.send(reply);
                        sender.close();
                    }
                }
            }
            session.sync();
        }
        catch (const std::exception &error) {
            std::cerr << "ConnectionService::HandlerRunning error:" << error.what();
        }

        try {
            // 为了解决应答sender队列被销毁后异常造成receiver.fetch异常
            session.close();
        }
        catch (const std::exception &error) {
            std::cerr << "HandlerRunning close error:" << error.what();
        }
    }
}

bool ConnectionService::PostMsg(const QMsgPtr &msg, int second, const ResponseCallback &cb)
{
    if (!_connection->isOpen() || !msg) {
        return false;
    }

    bool result = false;
    msg->setMessageId(NewMessageId());
    msg->setReplyTo(_asyncReceiver.getAddress());
    try {
        {
            std::unique_lock<std::mutex> lock(_requestCacheMutex);
            RequestInfo info(second, msg, cb);
            _requestCache[msg->getMessageId()] = info;
        }
        _sender.send(*msg.get());
        result = true;
    } catch (const std::exception& error) {
        std::cerr << "ConnectionService::PostMsg error:" << error.what();
    }

    return result;
}

bool ConnectionService::SendMsg(const Message &requestMsg, Message &responseMsg, int milliseconds)
{
    if (!_connection->isOpen()) {
        return false;
    }

    bool result = false;
    Message &msg = const_cast<Message&>(requestMsg);
    msg.setMessageId(NewMessageId());
    msg.setReplyTo(_syncReceiver.getAddress());
    try {
        _sender.send(msg);
        result = _syncReceiver.fetch(responseMsg, Duration(milliseconds));
        _session.acknowledge(responseMsg);
    }
    catch (const std::exception &error) {
        std::cerr << "ConnectionService::SendMsg error : " << error.what();
    }

    return result;
}
