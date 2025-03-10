#pragma once
#include <iostream>
#include "../common/Log.hpp"
#include "../common/message.hpp"
#include "../common/Message-type.hpp"
#include "request.hpp"
#include "../common/abstract.hpp"
#include <mutex>
#include <set>

namespace RPC
{
    namespace Client
    {
        class TopicManager
        {
        public:
            using ptr = std::shared_ptr<TopicManager>;
            using SubCallBack = std::function<void(const std::string &topic_name, const std::string &msg)>;
            TopicManager(const Requestor::ptr& req): _request(req){}
            bool TopicCreate(const BaseConnection::ptr &conn, const std::string &name)
            {
                return CommonTopic(TopicOptype::TOPIC_CREATE,conn, name);
            }
            bool TopicRemove(const BaseConnection::ptr &conn, const std::string &name)
            {
                return CommonTopic(TopicOptype::TOPIC_REMOVE,conn, name);
            }
            bool Subscribe(const BaseConnection::ptr &conn, const std::string &name, const SubCallBack &cb)
            {
                AddSubscribe(name,cb);
                bool ret =  CommonTopic(TopicOptype::TOPIC_SUBCRIBE,conn, name);
                if(!ret)
                {
                    RemoveSubscribe(name);
                    return false;
                }
                return true;
            }
            bool UnSubscribe(const BaseConnection::ptr &conn, const std::string &name)
            {
                RemoveSubscribe(name);
                return CommonTopic(TopicOptype::TOPIC_CANCEL,conn,name);
            }
            bool OnPublishMessage(const BaseConnection::ptr &conn, const std::string &name, const std::string &msg)
            {
                CommonTopic(TopicOptype::TOPIC_PUBLISH,conn,name,msg);
            }

            //处理发布消息的回调，作为订阅者订阅以后再遇到发布消息是时处理消息的会回调
            void OnPublish(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                auto type = msg->Topictype();
                if(type != TopicOptype::TOPIC_PUBLISH)
                {
                    DLOG("消息类型异常");
                }

                std::string topic_name = msg->TopicKey();
                std::string topic_msg = msg->TopicMsg();
                auto it = _rsp_callback.find(topic_name);
                if(it == _rsp_callback.end())
                {
                    ELOG("不存在当前主题的回调处理函数");
                    return;
                }
               return it->second(topic_name,topic_msg);
            }

        private:
            void AddSubscribe(const std::string& name, const SubCallBack& cb)
            {
                std::unique_lock<std::mutex>(_mutex);
                _rsp_callback.insert(std::make_pair(name,cb));
            }
            void RemoveSubscribe(const std::string& name)
            {
                std::unique_lock<std::mutex>(_mutex);
                _rsp_callback.erase(name);
            }
            const SubCallBack& GetSubscribe(const std::string& name)
            {
                std::unique_lock<std::mutex>(_mutex);
                auto it = _rsp_callback.find(name);
                if(it == _rsp_callback.end())
                {
                    return SubCallBack();
                }
                return it->second;
            } 
            bool CommonTopic(TopicOptype type, const BaseConnection::ptr &conn, const std::string &name, const std::string &msg = "")
            {
                auto req_message = MessageFactory::create<TopicRequest>();
                req_message->setId(UUID::uuid());
                req_message->setTopictype(type);
                req_message->setTopicKey(name);
                req_message->setMtype(Mtype::REQ_TOPIC);
                if(type == TopicOptype::TOPIC_PUBLISH)
                {
                    req_message->setTopicMsg(msg);
                }
                // 2.发送请求
                BaseMessage::ptr rsp_msg;
                bool ret = _request->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_message), rsp_msg);
                if (!ret)
                {
                    ELOG("发送同步Topic请求失败");
                    return false;
                }
                //DLOG("开始等待响应");
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
                return true;
            }

        private:
            std::mutex _mutex;
            Requestor::ptr _request;
            std::unordered_map<std::string, SubCallBack> _rsp_callback; // 管理不同主题订阅对应的处理函数
        };
    }

}
