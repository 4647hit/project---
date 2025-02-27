#include "net.hpp"
#include "Log.hpp"
#include "abstract.hpp"
#include <iostream>
#include "../client/request.hpp"
#include "Dispather.hpp"
#include "../client/rpc-caller.hpp"
// void OnMessage(const RPC::BaseConnection::ptr &conn, RPC::BaseMessage::ptr &msg)
// {
//     std::string body = msg->serialize();
//     std::cout << "body content : "<<body << std::endl;
// }

// void OnRpcrequest(const RPC::BaseConnection::ptr &conn, RPC::BaseMessage::ptr &msg)
// {
//     ILOG("recive imformation for client:");
//     std::cout << msg->serialize() << std::endl;
// }
// void OnTopicrequest(const RPC::BaseConnection::ptr &conn, RPC::BaseMessage::ptr &msg)
// {
//     ILOG("recive imformation for client:");
//     std::cout << msg->serialize() << std::endl;
// }

// void OnRpcresponse(const RPC::BaseConnection::ptr &conn, RPC::RpcResponse::ptr &msg)
// {
//     DLOG("开始处理消息");
//     std::string body = msg->serialize();
//     std::cout << "body content: " << body << std::endl;

// }
// void OnTopicresponse(const RPC::BaseConnection::ptr &conn, RPC::TopicResponse::ptr &msg)
// {
//     DLOG("开始处理消息");
//     std::string body = msg->serialize();
//     std::cout << "body content: " << body << std::endl;
// }
int main()
{
    auto requestor = std::make_shared<RPC::Client::Requestor>();
    auto RpcCaller = std::make_shared<RPC::Client::RpcCaller>(requestor);

    auto dispather = std::make_shared<Dispather>();

    auto req = RPC::ClientFactory::create("127.0.0.1", 9090);
    auto rsp_cb = std::bind(&Client::Requestor::onResponse,requestor.get(),std::placeholders::_1,std::placeholders::_2);
    dispather->registerhandle<RPC::RpcResponse> (Mtype::RSP_RPC, rsp_cb);
    auto message_cb = std::bind(&Dispather::OnMessage, dispather.get(), std::placeholders::_1, std::placeholders::_2);
    req->setMessageCallBack(message_cb);
    req->connect();

    auto it = req->connection();
    Json::Value params, result;
    params["num1"] = 11;
    params["num2"] = 22;
    bool ret = RpcCaller->call(it,"Add",params,result);
    if(ret != false)
    {
      std::cout << "result : " << result.asInt() << std::endl;
    }
    req->shutdown();
    // std::cout << "发出请求  --- 2" << std::endl;
    // auto rsp_t1 = RPC::MessageFactory::create<RPC::RpcRequest>();
    // rsp_t1->setId("32000");
    // rsp_t1->setMtype(Mtype::REQ_RPC);
    // rsp_t1->setMethod("delete");
    // Json::Value param;
    // param["num1"] = 1;
    // param["num2"] = 2;
    // rsp_t1->setParams(param);
    // req->send(rsp_t1);
    // std::cout << "发出请求  --- 1" << std::endl;
    // auto rsp_t = RPC::MessageFactory::create<RPC::TopicRequest>();
    // rsp_t->setId("5000");
    // rsp_t->setMtype(Mtype::REQ_TOPIC);
    // rsp_t->setTopictype(TopicOptype::TOPIC_CREATE);
    // rsp_t->setTopicMsg("this is a news");
    // req->send(rsp_t);


    // sleep(10);
    // req->shutdown();
    return 0;
}