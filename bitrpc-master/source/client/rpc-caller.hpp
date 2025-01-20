#include "request.hpp"

namespace RPC
{
    namespace Client
    {
        class RpcCaller
        {
            using ptr = std::shared_ptr<RpcCaller>;
            using JsonAsyncResponse = std::future<Json::Value>;
            using JsonResponseCallback = std::function<void(const Json::Value)>;
            RpcCaller(const RpcRequest::ptr& it): _RpcCall(it){}
            void send(const BaseConnection &conn, const std::string &name, const Json::Value &params, Json::Value &result)
            {
            }
            void send(const BaseConnection &conn, const std::string &name, const Json::Value &params,  JsonAsyncResponse &result)
            {
            }
            void send(const BaseConnection &conn, const std::string &name, const Json::Value &params,  JsonResponseCallback &result)
            {
            }

        private:
            RpcRequest::ptr _RpcCall;
        };
    }
}