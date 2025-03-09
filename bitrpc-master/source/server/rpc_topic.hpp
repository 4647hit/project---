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
            using ptr = std::shared_ptr<TopicManager>;
            void OnTopicRequest(const BaseConnection::ptr &conn, RPC::TopicRequest::ptr &request);

            struct Topic
            {
                using ptr = std::shared_ptr<Topic>;
                Topic(const std::string& topic_name):_topic_name(topic_name){}
                std::string _topic_name;
                std::set<BaseConnection::ptr> _subscribers;

                std::string Topicname();
                void putMessage(const BaseMessage::ptr& msg);
                void removeSub(const BaseConnection::ptr sub);
                void insertSub(const BaseConnection::ptr sub);
            };
            struct Subscriber
            {
                using ptr = std::shared_ptr<Subscriber>;
                Subscriber(const BaseConnection::ptr& conn):_conn(conn){}
                BaseConnection::ptr _conn;
                std::set<std::string> _topics;//已经订阅的主题名单

                void removeSub(const std::string& topic);
                void insertSub(const std::string& topic);
            };
            private:
            std::mutex _mutex;
            std::unordered_map<std::string,Topic::ptr> _topic;
            std::unordered_map<BaseConnection::ptr, Subscriber::ptr> _sub;
        };
    }
}