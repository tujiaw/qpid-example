#pragma once

#include "qpid/messaging/Session.h"
#include "qpid/messaging/Message.h"
#include "qpid/messaging/Sender.h"
#include "qpid/messaging/Receiver.h"

#include <functional>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <queue>
#include <time.h>

using namespace qpid::messaging;
using QMsgPtr = std::shared_ptr<Message>;

time_t GetCurrentTimeSec();
std::string NewMessageId();

class ConnectionService
{
public:
    using ResponseCallback = std::function<void(const QMsgPtr &sendMsg, const QMsgPtr &resMsg)>;
    using ServerCallback = std::function<void(const Message &msg, Message &reply)>;

    struct RequestInfo {
        RequestInfo() 
            : second(1)
            , time(GetCurrentTimeSec())
            , msg(nullptr)
            , cb(nullptr)
        {}

        RequestInfo(int s, const QMsgPtr &m, const ResponseCallback &c) 
            : second(s)
            , time(GetCurrentTimeSec())
            , msg(m)
            , cb(c)
        {}

        int second;
        time_t time;
        QMsgPtr msg;
        ResponseCallback cb;
    };

    ConnectionService(const std::string &url, const std::string &sendAddr);
    ConnectionService(const ConnectionService &) = delete;
    void operator=(const ConnectionService &) = delete;
    ~ConnectionService();

    void AddHandler(const std::string &serverAddr, const ServerCallback &cb);
    bool PostMsg(const QMsgPtr &msg, int second, const ResponseCallback &cb);
    bool SendMsg(const Message &requestMsg, Message &responseMsg, int milliseconds = 1000);

private:
    bool Open(const std::string& url);
    void Close();
    void StartThread();
    void ReceiveRunning();
    void TimeoutRunning();
    void HandlerRunning(const std::string &addr, const ServerCallback &cb);

private:
    std::unique_ptr<qpid::messaging::Connection> _connection;
    std::string _requestAddr;
    bool _is_open;
    Session _session;
    Sender _sender;
    Receiver _asyncReceiver;
    Receiver _syncReceiver;

    std::unique_ptr<std::thread> _receiveThread;
    std::vector<std::shared_ptr<std::thread>> _handlerThreadList;

    std::mutex _requestCacheMutex;
    std::unordered_map<std::string, RequestInfo> _requestCache;
};

