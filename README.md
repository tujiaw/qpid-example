qpid C++接口简单封装

希望简单的封装能满足大部分场景，并且简单易用。

# 处理队列请求
如果我想处理某个队列的消息我只需要指定一个队列名和回调处理函数就可以了
```
server.AddQueueServer("pingpong", [](const Message &msg, Message &reply) {
    std::cout << std::this_thread::get_id() << " msgid:" << msg.getMessageId() << ",reply:" << msg.getContent() << std::endl;
    reply = msg;
});
```
如果一个处理不过来可以循环调用上面函数多次，每个新增都是在一个单独的线程上处理，相当于多个线程在消费qpid上的消息

# 接收订阅消息
跟处理队列类似，订阅没有应答
```
server.AddTopicServer("ningtotopic", [](const Message &msg) {
    std::cout << std::this_thread::get_id() << " msgid:" << msg.getMessageId() << ",reply:" << msg.getContent() << std::endl;
});
```

# 发送同步消息
参数依次是：队列名、请求消息、应答消息、超时时间
```
bool SendMsg(const std::string &name, const Message &requestMsg, Message &responseMsg, int milliseconds = 1000);
```

# 发送异步消息
参数依次是：队列名、请求消息、超时时间秒、应答回调
```
bool PostMsg(const std::string &name, const QMsgPtr &msg, int second, const ResponseCallback &cb);
```

# 推送消息
参数依次是：topic、发送的消息
```
bool PublishMsg(const std::string &topic, const Message &msg);
```

# 源码
[github源码](https://github.com/tujiaw/qpid-example)

