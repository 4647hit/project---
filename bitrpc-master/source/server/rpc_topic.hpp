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
            void OnTopicRequest(const BaseConnection::ptr &conn, RPC::TopicRequest::ptr &request);

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
                            if(it_sub != _sub.end())
                            {
                                _subscribers.insert(it_sub->second);
                            }
                        }
                    }
                    _topic.erase(name);
                }
                for(auto sub :_subscribers)
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
                    if (it->second)
                    {
                        topic->insertSub(conn);
                    }
                    auto it_sub = _sub.find(conn);
                    if(it_sub == _sub.end())
                    {
                        auto newsub = std::make_shared<Subscriber>(conn);
                        if(newsub)
                        {
                            _sub.insert(std::make_pair(conn,newsub));
                        }
                        else
                        {
                            DLOG("构造新订阅者失败");
                        }
                    }

                }
            }

        private:
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

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, Topic::ptr> _topic;//每个名称对应的服务主题指针
            std::unordered_map<BaseConnection::ptr, Subscriber::ptr> _sub;//每个连接对应的订阅者信息
        };
    }
}