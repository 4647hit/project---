#include "net.hpp"
#include "Log.hpp"
#include "abstract.hpp"
#include "Message-type.hpp"
#include "Dispather.hpp"
#include <iostream>
// void OnMessage(const RPC::BaseConnection::ptr &conn, RPC::BaseMessage::ptr &msg)
// {
//     DLOG("开始处理消息");
//     std::string body = msg->serialize();
//     std::cout << "body content: " << body << std::endl;

//     auto rsp_t = RPC::MessageFactory::create<RPC::RpcResponse>();
//     rsp_t->setId("10");
//     rsp_t->setMtype(Mtype::RSP_RPC);
//     rsp_t->setRCode(RCode::RCODE_OK);
//     rsp_t->setResult("217390");
//     conn->send(rsp_t);
// }

void OnRpcrequest(const RPC::BaseConnection::ptr &conn, RPC::RpcRequest::ptr &msg)
{
    ILOG("recive imformation for client:");
    std::cout << msg->serialize() << std::endl;
    auto rsp_t = RPC::MessageFactory::create<RPC::RpcResponse>();
    rsp_t->setId("10");
    rsp_t->setMtype(Mtype::RSP_RPC);
    rsp_t->setRCode(RCode::RCODE_OK);
    rsp_t->setResult("217390");
    conn->send(rsp_t);
}
void OnTopicrequest(const RPC::BaseConnection::ptr &conn, RPC::TopicRequest::ptr &msg)
{
    ILOG("recive imformation for client:");
    std::cout << msg->serialize() << std::endl;
    auto rsp_t = RPC::MessageFactory::create<RPC::TopicResponse>();
    rsp_t->setId("10");
    rsp_t->setMtype(Mtype::RSP_TOPIC);
    rsp_t->setRCode(RCode::RCODE_OK);
    conn->send(rsp_t);
}
int main()
{
    auto dispather = std::make_shared<Dispather>();
    auto it = RPC::ServerFactory::create(9090);
    dispather->registerhandle< RPC::RpcRequest> (Mtype::REQ_RPC, OnRpcrequest);
    dispather->registerhandle< RPC::TopicRequest> (Mtype::REQ_TOPIC, OnTopicrequest);
    auto message_cb = std::bind(&Dispather::OnMessage, dispather.get(), std::placeholders::_1, std::placeholders::_2);

    it->setMessageCallBack(message_cb); //???为啥要加const
    it->start();
    return 0;
}