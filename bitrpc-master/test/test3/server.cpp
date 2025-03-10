#include "../../source/server/rpc_server.hpp"
#include <iostream>

int main()
{
    auto server = std::make_shared<RPC::Server::TopicCenter>(7070);
    server->Start();
    return 0;
}