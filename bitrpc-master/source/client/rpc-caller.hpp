#pragma once
#include "request.hpp"

namespace RPC
{
    namespace Client
    {
        class RpcCaller
        {
        public:
            using ptr = std::shared_ptr<RpcCaller>;
            using JsonAsyncResponse = std::future<Json::Value>;
            using JsonResponseCallback = std::function<void(const Json::Value)>;
            RpcCaller(const Requestor::ptr &it) : _RpcCall(it) {}
            // 同步请求
            bool call(const BaseConnection::ptr &conn, const std::string &method_name, const Json::Value &params, Json::Value &result)
            {
                // 1.组织请求

                auto req_message = MessageFactory::create<RpcRequest>();
                req_message->setId(UUID::uuid());
                req_message->setMethod(method_name);
                req_message->setParams(params);
                req_message->setMtype(Mtype::REQ_RPC);
                // 2.发送请求
                BaseMessage::ptr rsp_msg;

                bool ret = _RpcCall->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_message), rsp_msg);
                if (!ret)
                {
                    ELOG("发送同步RPC请求失败");
                    return false;
                }
                DLOG("开始等待响应");
                // 3.等待响应
                auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(rsp_msg);
                if (!rpc_rsp_msg)
                {
                    ELOG("向下转换失败");
                    return false;
                }
                if (rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                {
                    ELOG("rpc请求出错");
                    return false;
                }
                DLOG("等待响应成功");
                result = rpc_rsp_msg->result();
                return true;
            }
            bool call(const BaseConnection::ptr &conn, const std::string &method_name, const Json::Value &params, JsonAsyncResponse &result)
            {
                // 1.设置请求
                auto req_message = MessageFactory::create<RpcRequest>();
                req_message->setId(UUID::uuid());
                req_message->setMethod(method_name);
                req_message->setParams(params);
                req_message->setMtype(Mtype::REQ_RPC);
                // 2.发送请求
                DLOG("=======================");
                auto json_promise = std::make_shared<std::promise<Json::Value>>();
                result = json_promise->get_future();

                Requestor::RequestCallback cb = std::bind(&RpcCaller::Callback,
                                                          this, std::placeholders::_1, json_promise);
                DLOG("=======================");
                bool ret = _RpcCall->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_message), cb);
                DLOG("=======================");
                if (!ret)
                {
                    ELOG("异步发送请求失败");
                    return false;
                }
                return true;
            }
            bool call(const BaseConnection::ptr &conn, const std::string &method_name, const Json::Value &params, const JsonResponseCallback &cb)
            {
                // 1.设置请求
                auto req_message = MessageFactory::create<RpcRequest>();
                req_message->setId(UUID::uuid());
                req_message->setMethod(method_name);
                req_message->setParams(params);
                req_message->setMtype(Mtype::REQ_RPC);
                // 2.发送请求

                auto json_promise = std::shared_ptr<std::promise<Json::Value>>();
                Requestor::RequestCallback req_cb = std::bind(&RpcCaller::Callback1,
                                                              this, std::placeholders::_1, cb);
                bool ret = _RpcCall->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_message), req_cb);
                if (!ret)
                {
                    ELOG("回调发送请求失败");
                    return false;
                }
                return true;
            }

        private:
            void Callback1(const BaseMessage::ptr &msg, JsonResponseCallback &cb)
            {
                auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                if (!rpc_rsp_msg)
                {
                    ELOG("向下转换失败");
                    return;
                }
                if (rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                {
                    ELOG("rpc异步请求出错");
                    return;
                }
                cb(rpc_rsp_msg->result());
            }
            void Callback(const BaseMessage::ptr &msg, std::shared_ptr<std::promise<Json::Value>> &result)
            {
                auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                if (!rpc_rsp_msg)
                {
                    ELOG("向下转换失败");
                    return;
                }
                if (rpc_rsp_msg->rcode() != RCode::RCODE_OK)
                {
                    ELOG("rpc异步请求出错");
                    return;
                }
                result->set_value(rpc_rsp_msg->result());
            }

        private:
            Requestor::ptr _RpcCall;
        };
    }
}