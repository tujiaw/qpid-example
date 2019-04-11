
#include "ConnectionService.h"

#include <iostream>
#include <sstream>
#include <set>
#include <iomanip>
#include <time.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <unordered_set>

using namespace qpid::messaging;

int main()
{
    ConnectionService server("172.16.66.115:5672");
    
#if 0
    //server.AddQueueServer("pingpong", [](const Message &msg, Message &reply) {
    //    std::cout << std::this_thread::get_id() << " msgid:" << msg.getMessageId() << ",reply:" << msg.getContent() << std::endl;
    //    reply = msg;
    //});


    server.AddTopicServer("ningtotopic", [](const Message &msg) {
        std::cout << std::this_thread::get_id() << " msgid:" << msg.getMessageId() << ",reply:" << msg.getContent() << std::endl;
    });

#else
    std::vector<int> postTimeoutList, sendTimeoutList;
    int i = 0;
    for (;;) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        qpid::types::Variant::Map mp;
        mp["1"] = "111111";
        mp["2"] = "sumscope";
        mp["3"] = 200;

        QMsgPtr msg(new Message());
        msg->setMessageId(NewMessageId());
        msg->setCorrelationId("getCompanyInfo");
        msg->setContentObject(mp);
        msg->setContent("hello");

        //server.PostMsg("pingpong", msg, 5, [&](const QMsgPtr &request, const QMsgPtr &response) {
        //    if (response->getContentSize() > 0) {
        //        std::cout << "request:" << request->getContent() << ", response:" << response->getContent() << std::endl;
        //    } else {
        //        postTimeoutList.push_back(i);
        //    }
        //});

        std::cout << "publish msg:" << i << std::endl;
        server.PublishMsg("broker.IDC.bondQuote.hellofdg", *msg.get());
    }

#endif

    system("pause");
    return 0;
}