#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Session.h>
#include "ConnectionService.h"

#include <iostream>
#include <sstream>

using namespace qpid::messaging;

int main()
{
    //ConnectionService server("118.24.4.114:5672", "pingpong");
    ConnectionService server("172.16.66.115:5672");

#if 0
    server.AddQueueServer("pingpong", [](const Message &msg, Message &reply) {
        std::cout << std::this_thread::get_id() << " msgid:" << msg.getMessageId() << ",reply:" << msg.getContent() << std::endl;
        reply.setContent(msg.getContent());
    });

#else
    std::vector<int> postTimeoutList, sendTimeoutList;
    for (int i = 0; i < 100; i++) {
        QMsgPtr msg(new Message());
        msg->setContent(std::to_string(i));
        //server.PostMsg("pingpong", msg, 13, [&](const QMsgPtr &request, const QMsgPtr &response) {
        //    if (response->getContentSize() > 0) {
        //        std::cout << "request:" << request->getContent() << ", response:" << response->getContent() << std::endl;
        //    } else {
        //        postTimeoutList.push_back(i);
        //    }
        //});

        server.PublishMsg("ningtotopic; {create:always, node : {type: topic}}", *msg.get());

        //Message responseMsg;
        //if (server.SendMsg(*msg.get(), responseMsg)) {
        //    std::cout << responseMsg.getContent() << std::endl;
        //} else {
        //    sendTimeoutList.push_back(i);
        //}
    }


    system("pause");
    std::cout << "=====================================\n";
    if (!postTimeoutList.empty()) {
        std::cout << "post timeout size:" << postTimeoutList.size() << std::endl;
    }
    if (!sendTimeoutList.empty()) {
        std::cout << "send timeout size:" << sendTimeoutList.size() << std::endl;
    }

#endif

    system("pause");
    return 0;
}