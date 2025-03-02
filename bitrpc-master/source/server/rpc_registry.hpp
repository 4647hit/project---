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
    namespace Server
    {
        class ProviderManager
        {
        public:
            using ptr = std::shared_ptr<ProviderManager>;
            struct Provider
            {
                using ptr = std::shared_ptr<Provider>;
                Address host;
                std::mutex _mutex;
                std::vector<std::string> methods;
                BaseConnection::ptr conn;
                void append(std::string method)
                {
                    std::unique_lock<std::mutex>(_mutex);
                    methods.emplace_back(method);
                }
                Provider(const Address h, const BaseConnection::ptr &c) : host(h), conn(c)
                {
                }
            };
            // 添加提供者信息
            void addProvider(const BaseConnection::ptr conn, const Address &_host, std::string methods)
            {
                Provider::ptr _pro;
                // 查找原来该提供者是否提供过服务；
                {
                    std::unique_lock<std::mutex>(__mutex);
                    auto connection = _conns.find(conn);
                    // 原来是否有该提供者
                    if (connection != _conns.end())
                    {
                        _pro = connection->second;
                    }
                    else
                    {
                        _pro = std::make_shared<Provider>(_host, conn);
                        _conns.insert(std::make_pair(conn, _pro));
                    }
                    auto &provider_set = _provider[methods];
                    provider_set.insert(_pro);
                }
                _pro->append(methods);
            }
            // 查询提供者的信息，方便对发现者进行通知
            Provider::ptr getProvider(const BaseConnection::ptr &conn)
            {
                std::unique_lock<std::mutex>(__mutex);
                auto it = _conns.find(conn);
                if (it != _conns.end())
                {
                    return it->second;
                }
                return Provider::ptr();
            }
            // 删除提供者
            void delProvider(const BaseConnection::ptr &conn)
            {
                std::unique_lock<std::mutex>(__mutex);
                auto it = _conns.find(conn);
                if (it == _conns.end())
                {
                    return;
                }
                for (auto &meth : it->second->methods)
                {
                    auto &its = _provider[meth];
                    its.erase(it->second);
                }
                _conns.erase(conn);
            }

        private:
            std::mutex __mutex;
            std::unordered_map<std::string, std::set<Provider::ptr>> _provider;
            std::unordered_map<BaseConnection::ptr, Provider::ptr> _conns;
        };
        class DiscoverManager
        {
        public:
            using ptr = std::shared_ptr<DiscoverManager>;
            struct Discover
            {
                using ptr = std::shared_ptr<Discover>;
                std::mutex _mutex;
                std::vector<std::string> methods;
                BaseConnection::ptr conn;
                void append(std::string method)
                {
                    std::unique_lock<std::mutex>(_mutex);
                    methods.emplace_back(method);
                }
                Discover(const BaseConnection &c)
                {
                }
            };
            // 添加发现者信息
            Discover::ptr addDiscover(const BaseConnection::ptr &conn, const std::string &method)
            {
                Discover::ptr _pro;
                // 查找原来是否有发现者
                {
                    std::unique_lock<std::mutex>(__mutex);
                    auto connection = _conns.find(conn);
                    // 原来是否有该发现者
                    if (connection != _conns.end())
                    {
                        _pro = connection->second;
                    }
                    else
                    {
                        _pro = std::make_shared<Discover>(conn);
                        _conns.insert(std::make_pair(conn, _pro));
                    }
                    auto &provider_set = _discover[method];
                    provider_set.insert(_pro);
                }
                _pro->append(method);
            }
            // 删除提供者
            void delDiscover(const BaseConnection::ptr &conn)
            {
                std::unique_lock<std::mutex>(__mutex);
                auto it = _conns.find(conn);
                if (it == _conns.end())
                {
                    return;
                }
                for (auto &meth : it->second->methods)
                {
                    auto &its = _discover[meth];
                    its.erase(it->second);
                }
                _conns.erase(conn);
            }
            // 上线通知
            void online_Notifi(const std::string &method,const Address& host)
            {
                return notify(method,host,ServiceOptype::SERVICE_ONLINE);
            }
            // 下线通知
            void offline_Notifi(const std::string &method,const Address& host)
            {
                return notify(method,host,ServiceOptype::SERVICE_OFFLINE);
            }
        private:
            void notify(const std::string &method,const Address& host,ServiceOptype type)
            {
                std::unique_lock<std::mutex>(__mutex);
                auto it = _discover.find(method);
                if(it == _discover.end())
                {
                    return;
                }
                auto& discover_set = it->second;
                auto service_msg = MessageFactory::create<ServiceRequest>();
                service_msg->setId(RPC::UUID::uuid());
                service_msg->setMtype(Mtype::REQ_SERVICE);
                service_msg->setServiceMethod(method);
                service_msg->setHost(host);
                service_msg->setServicetype(type);
                auto& discover_set = it->second;
                for(auto& its : discover_set)
                {
                    its->conn->send(service_msg);
                }
            }

        private:
            std::mutex __mutex;
            std::unordered_map<std::string, std::set<Discover::ptr>> _discover;
            std::unordered_map<BaseConnection::ptr, Discover::ptr> _conns;
        };

        class PDManager
        {
            using ptr = std::shared_ptr<PDManager>;
            void OnServiceRequest(const BaseConnection::ptr &conn, const ServiceRequest &req);
            void Shutdown(const BaseConnection::ptr &conn);
            PDManager()
            {
            }

        private:
            DiscoverManager _disc;
            ProviderManager _proc;
        };
    }
}
