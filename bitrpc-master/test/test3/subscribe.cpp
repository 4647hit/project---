#include "../../source/client/rpc_client.hpp"
#include <iostream>
#include <thread>   // 提供 std::this_thread::sleep_for
#include <chrono>   // 提供时间单位（如 std::chrono::seconds
void callback(const std::string& name,const std::string& message)
{
    DLOG("订阅的%s主题发来%s消息",name.c_str(),message.c_str());
}
int main()
{
    auto publish = std::make_shared<RPC::Client::TopicClient>("127.0.0.1",7070);
    if(publish == nullptr)
    {
        DLOG("创建对象失败");
        return -1;
    }
   //3. 订阅主题
   bool ret = publish->Subscribe("hello", callback);
   //4. 等待->退出
   std::this_thread::sleep_for(std::chrono::seconds(10));
   publish->Shutdown();
    return 0;
}