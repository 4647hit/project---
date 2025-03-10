#include "../../source/client/rpc_client.hpp"
#include <iostream>
#include <thread>   // 提供 std::this_thread::sleep_for
#include <chrono>   // 提供时间单位（如 std::chrono::seconds
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
    std::this_thread::sleep_for(std::chrono::seconds(5));
    for (int i = 0; i < 10 ; i++)
    {
        publish->OnPublishMessage("hello","Hello - world - " + std::to_string(i));
    }

    return 0;
}