
#include "../source/common/Log.hpp"
#include "../source/common/net.hpp"
#include "../source/common/abstract.hpp"
#include "../source/common/Message-type.hpp"
#include "../source/common/Dispather.hpp"
#include <iostream>
#include "../source/server/rpc_route.hpp"
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

// void OnRpcrequest(const RPC::BaseConnection::ptr &conn, RPC::RpcRequest::ptr &msg)
// {
//     ILOG("recive imformation for client:");
//     std::cout << msg->serialize() << std::endl;
//     auto rsp_t = RPC::MessageFactory::create<RPC::RpcResponse>();
//     rsp_t->setId("10");
//     rsp_t->setMtype(Mtype::RSP_RPC);
//     rsp_t->setRCode(RCode::RCODE_OK);
//     rsp_t->setResult("217390");
//     conn->send(rsp_t);
// }
// void OnTopicrequest(const RPC::BaseConnection::ptr &conn, RPC::TopicRequest::ptr &msg)
// {
//     ILOG("recive imformation for client:");
//     std::cout << msg->serialize() << std::endl;
//     auto rsp_t = RPC::MessageFactory::create<RPC::TopicResponse>();
//     rsp_t->setId("10");
//     rsp_t->setMtype(Mtype::RSP_TOPIC);
//     rsp_t->setRCode(RCode::RCODE_OK);
//     conn->send(rsp_t);
// }
void Add(const Json::Value &req, Json::Value &rsp)
{
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    int result = num1 + num2;
    rsp = result;
}
int main()
{
    auto router = std::make_shared<RPC::Server::RpcRouter>();
    std::unique_ptr<RPC::Server::SDescribeFactory> desc_factory(new RPC::Server::SDescribeFactory());
    desc_factory->SetMethodname("Add");
    desc_factory->Setparams("num1", Server::Valuetype::INT);
    desc_factory->Setparams("num2", Server::Valuetype::INT);
    desc_factory->SetreturnValue(Server::Valuetype::INT);
    desc_factory->SetServerCallback(Add);
    router->registerMethod(desc_factory->build());
    
    auto cb = std::bind(&RPC::Server::RpcRouter::OnRpcRequest,router.get(),
    std::placeholders::_1,std::placeholders::_2);
    
    auto dispather = std::make_shared<Dispather>();
    dispather->registerhandle<RPC::RpcRequest>(Mtype::REQ_RPC, cb);
    auto message_cb = std::bind(&Dispather::OnMessage, dispather.get(),
     std::placeholders::_1, std::placeholders::_2);

    auto it = RPC::ServerFactory::create(9090);
    it->setMessageCallBack(message_cb); //???为啥要加const
    it->start();
    return 0;
}