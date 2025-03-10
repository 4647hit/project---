#pragma once
#include <iostream>
#include "../common/Log.hpp"
#include "../common/message.hpp"
#include "../common/Message-type.hpp"
#include "../common/abstract.hpp"
#include <mutex>
#include <set>

namespace RPC
{
    namespace server
    {
        class TopicManager
        {
        public:
            TopicManager() {}
            using ptr = std::shared_ptr<TopicManager>;
            void OnTopicRequest(const BaseConnection::ptr &conn, RPC::TopicRequest::ptr &request)
            {
                auto type = request->Topictype();
                bool ret1, ret2;
                {
                    switch (type)
                    {
                    case TopicOptype::TOPIC_CREATE:
                        TopicCreate(conn, request);
                        break;
                    case TopicOptype::TOPIC_REMOVE:
                        TopicRemove(conn, request);
                        break;
                    case TopicOptype::TOPIC_SUBCRIBE:
                        ret1 = TopicSubscribe(conn, request);
                        break;
                    case TopicOptype::TOPIC_CANCEL:
                        ret2 = TopicCancel(conn, request);
                        break;
                    case TopicOptype::TOPIC_PUBLISH:
                        PublishTopicMessage(request->TopicKey(), request);
                        break;

                    default:
                        Response(conn, request, RPC::RCode::RCODE_INVAILD_OPTYPE);
                        break;
                    }
                    if (!ret1)
                    {
                        return Response(conn, request, RPC::RCode::RCODE_NOT_FOUND_TOPIC); // 存在两种原因
                    }
                    if (!ret2)
                    {
                        return Response(conn, request, RPC::RCode::RCODE_NOT_FOUND_TOPIC);
                    }
                    return Response(conn, request, RPC::RCode::RCODE_OK);
                }
            }
            void OnShutdown(const BaseConnection::ptr &conn)
            {
                // 订阅者关闭连接，与提供者没有关系
                std::unique_lock<std::mutex>(_mutex);
                auto it = _sub.find(conn);
                Subscriber::ptr close_sub;
                if (it != _sub.end())
                {
                    // 可能会出现问题
                    close_sub = it->second;
                    for (auto topic : close_sub->_topics)
                    {
                        auto tp = _topic.find(topic);
                        if (tp != _topic.end())
                        {
                            tp->second->removeSub(conn);
                        }
                    }
                }
                _sub.erase(conn);
            }

        private:
            void Response(const BaseConnection::ptr &conn, const TopicRequest::ptr &req, RPC::RCode ReturnCode = RPC::RCode::OK)
            {
                auto service_msg = MessageFactory::create<TopicResponse>();
                service_msg->setId(req->id());
                service_msg->setMtype(Mtype::RSP_TOPIC);
                service_msg->setRCode(ReturnCode);
                return conn->send(service_msg);
            }

        private:
            void TopicCreate(const BaseConnection::ptr &conn, RPC::TopicRequest::ptr &request)
            {

                auto name = request->TopicKey();
                std::unique_lock<std::mutex>(_mutex);
                auto topic = std::make_shared<Topic>(name);
                _topic.insert(std::make_pair(name, topic));
            }
            void TopicRemove(const BaseConnection::ptr &conn, RPC::TopicRequest::ptr &request)
            {
                // 1、先找到受主题删除影响的订阅者
                std::string name = request->TopicKey();
                std::set<Subscriber::ptr> _subscribers;
                {
                    std::unique_lock<std::mutex>(_mutex);
                    auto it = _topic.find(name);
                    if (it == _topic.end())
                    {
                        DLOG("未找到当前的服务");
                        return;
                    }
                    if (it->second)
                    {
                        for (auto &subscribers : it->second->_subscribers)
                        {
                            auto it_sub = _sub.find(subscribers);
                            if (it_sub != _sub.end())
                            {
                                _subscribers.insert(it_sub->second);
                            }
                        }
                    }
                    _topic.erase(name); // 删除当前的主题映射关系
                }
                for (auto sub : _subscribers)
                {
                    sub->removeTopic(name);
                }
            }
            bool TopicSubscribe(const BaseConnection::ptr &conn, RPC::TopicRequest::ptr &request)
            {
                Topic::ptr topic;
                Subscriber::ptr _subscriber;
                std::string name = request->TopicKey();
                {
                    std::unique_lock<std::mutex>(_mutex);
                    auto it = _topic.find(name);
                    if (it == _topic.end())
                    {
                        DLOG("未找到当前的主题");
                        return false;
                    }
                    // 插入订阅
                    if (it->second)
                    {
                        it->second->insertSub(conn);
                    }
                    auto it_sub = _sub.find(conn);
                    if (it_sub == _sub.end())
                    {
                        auto newsub = std::make_shared<Subscriber>(conn);
                        if (newsub)
                        {
                            newsub->insertTopic(name);
                            _sub.insert(std::make_pair(conn, newsub));
                        }
                        else
                        {
                            DLOG("构造新订阅者失败");
                            return false;
                        }
                    }
                    else
                    {
                        it_sub->second->insertTopic(name);
                    }
                }
                return true;
            }
            bool TopicCancel(const BaseConnection::ptr &conn, RPC::TopicRequest::ptr &request)
            {
                Subscriber::ptr _subscriber;
                std::string name = request->TopicKey();
                {
                    std::unique_lock<std::mutex>(_mutex);
                    auto it = _topic.find(name);
                    if (it == _topic.end())
                    {
                        DLOG("未找到当前的主题");
                        return false;
                    }
                    // 清除订阅
                    if (it->second)
                    {
                        it->second->removeSub(conn);
                    }
                    auto it_sub = _sub.find(conn);
                    if (it_sub == _sub.end())
                    {
                        DLOG("数据异常，未找到订阅者的喜爱相关信息");
                        return false;
                    }
                    else
                    {
                        it_sub->second->removeTopic(name);
                    }
                }
                return true;
            }
            void PublishTopicMessage(const std::string &topic_name, RPC::TopicRequest::ptr &msg)
            {
                {
                    std::unique_lock<std::mutex>(_mutex);
                    auto topic = _topic.find(topic_name);
                    if (topic != _topic.end())
                    {
                        for (auto it : topic->second->_subscribers)
                        {
                            it->send(msg);
                        }
                    }
                }
            }

        private:
            struct Subscriber
            {
                using ptr = std::shared_ptr<Subscriber>;
                Subscriber(const BaseConnection::ptr &conn) : _conn(conn) {}
                BaseConnection::ptr _conn;
                std::mutex _mutex;
                std::set<std::string> _topics; // 已经订阅的主题名单

                void removeTopic(const std::string &topic)
                {
                    std::unique_lock<std::mutex>(_mutex);
                    _topics.erase(topic);
                }
                void insertTopic(const std::string &topic)
                {
                    std::unique_lock<std::mutex>(_mutex);
                    _topics.insert(topic);
                }
            };
            struct Topic
            {
                using ptr = std::shared_ptr<Topic>;
                Topic(const std::string &topic_name) : _topic_name(topic_name) {}
                std::string _topic_name;
                std::set<BaseConnection::ptr> _subscribers;
                std::mutex _mutex;

                std::string Topicname()
                {
                    return _topic_name;
                }
                void putMessage(const BaseMessage::ptr &msg)
                {
                    std::unique_lock<std::mutex>(_mutex);
                    for (auto &subscribe : _subscribers)
                    {
                        subscribe->send(msg);
                    }
                }
                void removeSub(const BaseConnection::ptr sub)
                {
                    std::unique_lock<std::mutex>(_mutex);
                    _subscribers.erase(sub);
                }
                void insertSub(const BaseConnection::ptr sub)
                {
                    std::unique_lock<std::mutex>(_mutex);
                    _subscribers.insert(sub);
                }
            };

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, Topic::ptr> _topic;            // 每个名称对应的服务主题指针
            std::unordered_map<BaseConnection::ptr, Subscriber::ptr> _sub; // 每个连接对应的订阅者信息
        };
    }
}