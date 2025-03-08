#include <thread>
#include <chrono>
#include "../../source/common/Log.hpp"
#include "../../source/client/rpc_client.hpp"
#include <iostream>

void callback(const Json::Value &res)
{
  std::cout << "result : " << res.asInt() << std::endl;
}
int main()
{
  RPC::Client::RpcClient client(true, "127.0.0.1", 8080);
  Json::Value params, result;
  params["num1"] = 11;
  params["num2"] = 22;
  bool ret = client.call("Add", params, result); // ？？？？？？？？？发送流程
  if (ret == false)
  {
    std::cout << "调用失败" << std::endl;
  }
  std::cout << "result : " << result.asInt() << std::endl;

  Client::RpcCaller::JsonAsyncResponse result1;
  params["num1"] = 22;
  params["num2"] = 33;
   ret = client.call("Add", params, result1); // ？？？？？？？？？发送流程
  if (ret != false)
  {
    DLOG("=======================");
    result = result1.get();
    std::cout << "result : " << result.asInt() << std::endl;
  }

  params["num1"] = 44;
  params["num2"] = 14;
  ret = client.call("Add", params, callback); // ？？？？？？？？？发送流程

  std::this_thread::sleep_for(std::chrono::seconds(1));

  return 0;
}