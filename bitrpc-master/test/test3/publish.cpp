#include "../../source/client/rpc_client.hpp"

int main()
{
    auto publish = std::make_shared<RPC::Client::TopicClient>("127.0.0.1",7070);
    if(publish == nullptr)
    {
        DLOG("创建对象失败");
        return -1;
    }
    
    bool ret = publish->TopicCreate("hello");
    if(ret == false)
    {
        DLOG("创建主题失败");
    }

    for (int i = 0; i < 10 ; i++)
    {
        publish->OnPublishMessage("hello","Hello - world - " + std::to_string(i));
    }

    return 0;
}