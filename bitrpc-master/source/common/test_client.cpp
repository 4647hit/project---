#include "net.hpp"
#include "Log.hpp"
#include "abstract.hpp"
#include <iostream>

void OnMessage(const RPC::BaseConnection::ptr &conn, RPC::BaseMessage::ptr &msg)
{
    std::string body = msg->serialize();
    std::cout << body << std::endl;
}
int main()
{

    auto req = RPC::ClientFactory::create("127.0.0.1", 9090);
    req->setMessageCallBack(OnMessage);

    req->connect();
    auto rsp_t = RPC::MessageFactory::create<RPC::RpcRequest>();
    rsp_t->setId("37198");
    rsp_t->setMtype(Mtype::REQ_RPC);
    rsp_t->setMethod("add");
    Json::Value param;
    param["num1"] = 33;
    param["num2"] = 22;
    rsp_t->setParams(param);
    req->send(rsp_t);
    sleep(3);
    req->shutdown();
    return 0;
}