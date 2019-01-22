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
    using SubscribeCallback = std::function<void(const Message &msg)>;

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

    ConnectionService(const std::string &url);
    ConnectionService(const ConnectionService &) = delete;
    void operator=(const ConnectionService &) = delete;
    ~ConnectionService();

    void AddQueueServer(const std::string &serverAddr, const ServerCallback &cb);
    void AddTopicServer(const std::string &serverAddr, const SubscribeCallback &cb);
    bool PostMsg(const std::string &name, const QMsgPtr &msg, int second, const ResponseCallback &cb);
    bool SendMsg(const std::string &name, const Message &requestMsg, Message &responseMsg, int milliseconds = 1000);
    bool PublishMsg(const std::string &topic, const Message &msg);

private:
    bool Open(const std::string& url);
    void Close();
    void StartThread();
    void ReceiveRunning();
    void TimeoutRunning();
    void QueueServerRunning(const std::string &addr, const ServerCallback &cb);
    void TopicServerRunning(const std::string &addr, const SubscribeCallback &cb);

    Sender& GetSender(const std::string &name, const std::string &nodeType);

private:
    std::unique_ptr<qpid::messaging::Connection> _connection;
    bool _is_open;
    Session _session;
    Receiver _asyncReceiver;
    Receiver _syncReceiver;

    std::unique_ptr<std::thread> _receiveThread;
    std::vector<std::shared_ptr<std::thread>> _handlerThreadList;

    std::mutex _requestCacheMutex;
    std::unordered_map<std::string, RequestInfo> _requestCache;

    mutable std::mutex _senderMutex;
    std::unordered_map<std::string, Sender> _senderCache;
    Sender _emptySender;
};

