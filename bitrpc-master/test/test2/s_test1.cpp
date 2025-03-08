
#include "../../source/common/Log.hpp"
#include <iostream>
#include "../../source/server/rpc_server.hpp"
void Add(const Json::Value &req, Json::Value &rsp)
{
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    int result = num1 + num2;
    rsp = result;
}
int main()
{

    

    std::unique_ptr<RPC::Server::SDescribeFactory> desc_factory(new RPC::Server::SDescribeFactory());
    desc_factory->SetMethodname("Add");
    desc_factory->Setparams("num1", RPC::Server::Valuetype::INT);
    desc_factory->Setparams("num2", RPC::Server::Valuetype::INT);
    desc_factory->SetreturnValue(RPC::Server::Valuetype::INT);
    desc_factory->SetServerCallback(Add);
    RPC::Server::Route_Server server(Address("127.0.0.1",9090));

    server.RegisterMethod(desc_factory->build());
    server.Start();
    return 0;
}