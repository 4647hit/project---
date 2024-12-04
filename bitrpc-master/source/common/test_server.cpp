#include "net.hpp"
#include "Log.hpp"
#include "abstract.hpp"
#include "Message-type.hpp"
#include <iostream>
void OnMessage(const RPC::BaseConnection::ptr &conn, RPC::BaseMessage::ptr &msg)
{
    DLOG("开始处理消息");
    std::string body = msg->serialize();
    std::cout << "body content: " << body << std::endl;

    auto rsp_t = RPC::MessageFactory::create<RPC::RpcResponse>();
    rsp_t->setId("10");
    rsp_t->setMtype(Mtype::RSP_RPC);
    rsp_t->setRCode(RCode::RCODE_OK);
    rsp_t->setResult("217390");
    conn->send(rsp_t);
}
int main()
{
    auto it = RPC::ServerFactory::create(9090);
    it->setMessageCallBack(OnMessage); //???为啥要加const
    it->start();
    return 0;
}