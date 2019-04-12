qpid C++�ӿڼ򵥷�װ

ϣ���򵥵ķ�װ������󲿷ֳ��������Ҽ����á�

# �����������
������봦��ĳ�����е���Ϣ��ֻ��Ҫָ��һ���������ͻص��������Ϳ�����
```
server.AddQueueServer("pingpong", [](const Message &msg, Message &reply) {
    std::cout << std::this_thread::get_id() << " msgid:" << msg.getMessageId() << ",reply:" << msg.getContent() << std::endl;
    reply = msg;
});
```
���һ��������������ѭ���������溯����Σ�ÿ������������һ���������߳��ϴ����൱�ڶ���߳�������qpid�ϵ���Ϣ

# ���ն�����Ϣ
������������ƣ�����û��Ӧ��
```
server.AddTopicServer("ningtotopic", [](const Message &msg) {
    std::cout << std::this_thread::get_id() << " msgid:" << msg.getMessageId() << ",reply:" << msg.getContent() << std::endl;
});
```

# ����ͬ����Ϣ
���������ǣ���������������Ϣ��Ӧ����Ϣ����ʱʱ��
```
bool SendMsg(const std::string &name, const Message &requestMsg, Message &responseMsg, int milliseconds = 1000);
```

# �����첽��Ϣ
���������ǣ���������������Ϣ����ʱʱ���롢Ӧ��ص�
```
bool PostMsg(const std::string &name, const QMsgPtr &msg, int second, const ResponseCallback &cb);
```

# ������Ϣ
���������ǣ�topic�����͵���Ϣ
```
bool PublishMsg(const std::string &topic, const Message &msg);
```

# Դ��
[githubԴ��](https://github.com/tujiaw/qpid-example)

