#pragma once
#include "../common/Log.hpp"
#include "../common/message.hpp"
#include "../common/abstract.hpp"
#include <mutex>
#include <future>

namespace RPC
{
    namespace Client
    {
        class Requestor
        {
        public:
            using ptr = std::shared_ptr<Requestor>;
            using RequestCallback = std::function<void(const BaseMessage::ptr &)>;
            using AsyncResponse = std::future<BaseMessage::ptr>;
            struct RequestDescribe
            {
                using ptr = std::shared_ptr<RequestDescribe>;
                BaseMessage::ptr request;
                RType rtype;
                std::promise<BaseMessage::ptr> response;
                RequestCallback rb;
                void setCallback(const RequestCallback &cb)
                {
                    rb = cb;
                }
                AsyncResponse Asyncresponse()
                {
                    return response.get_future();
                }
            };
            void onResponse(const BaseConnection::ptr &conn, const BaseMessage::ptr &msg)
            {
                //printf("get response over!\n");
                std::string rid = msg->id();
                RequestDescribe::ptr it = this->getDescribe(rid);
                if (it == nullptr)
                {
                    ELOG("请求发送成功，但是找不到对应的服务描述，%s", rid.c_str());
                    return;
                }

                if (it->rtype == RType::REQ_ASYNC)
                {
                    it->response.set_value(msg);
                }
                else if (it->rtype == RType::REQ_CALLBACK)
                {
                    if (it->rb)
                    {
                        it->rb(msg);
                    }
                }
                else
                {
                    ELOG("出现未知的服务类型");
                }
                delDescribe(rid);
            }
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, AsyncResponse &async_rsp)
            {
                RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_ASYNC);
                if (rdp == nullptr)
                {
                    ELOG("构造请求描述对象失败");
                    return false;
                }
                conn->send(req);
                DLOG("------------------------------------");
                async_rsp = rdp->Asyncresponse();
                return true;
            }
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, BaseMessage::ptr &rsp)
            {
                AsyncResponse rsp_future;
                if (send(conn, req, rsp_future) == false)
                {
                    return false;
                }
                rsp = rsp_future.get();
                return true;
            }
            bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, RequestCallback &cb)
            {
                
                RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_CALLBACK, cb);
                DLOG("=======================");
                if (rdp.get() == nullptr)
                {
                    ELOG("构造请求描述对象失败");
                    return false;
                }
                conn->send(req);
                return true;
            }

        private:
            RequestDescribe::ptr newDescribe(const BaseMessage::ptr &req, RType type, const RequestCallback &callback = RequestCallback())
            {
                DLOG("--------------------------");
                std::unique_lock<std::mutex> lock(_mutex);
                DLOG("--------------------------");
                RequestDescribe::ptr rd = std::make_shared<RequestDescribe>();
                rd->rtype = type;
                rd->request = req;
                if (rd->rtype == RType::REQ_CALLBACK && callback)
                {
                    rd->setCallback(callback);
                }
                _requestor.insert(std::make_pair(req->id(), rd));
                return rd;
            }
            RequestDescribe::ptr getDescribe(const std::string &rid)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _requestor.find(rid);
                if (it == _requestor.end())
                {
                    return RequestDescribe::ptr();
                }
                return it->second;
            }
            void delDescribe(const std::string &rid)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _requestor.erase(rid);
            }
        private:
            std::mutex _mutex;
            std::unordered_map<std::string, RequestDescribe::ptr> _requestor;
        };
    }
}