#include "../../source/server/rpc_server.hpp"
#include "../../source/common/Log.hpp"


int main()
{
    Server::RegisterCenter req_server(8080);
    req_server.Start();
    return 0;
}