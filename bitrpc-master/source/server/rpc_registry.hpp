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
                public:
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
            std::vector<Address> GetAddress(const std::string& method)
            {
                auto it = _provider.find(method);
                if(it !=_provider.end())
                {
                    std::vector<Address> address;
                    for(auto& addr : it->second)
                    {
                        address.emplace_back(addr->host);
                    }
                    return address;
                }
                return std::vector<Address>();

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
                Discover(const BaseConnection::ptr &c)
                    : conn(c)
                {
                }
            };
            // 添加发现者信息
            void addDiscover(const BaseConnection::ptr &conn, const std::string &method)
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
            // 删除faxian者  --- 要是服务提供者下线了，但是每有提供服务的提供者了该如何操作？
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
            void online_Notifi(const std::string &method, const Address &host)
            {
                return notify(method, host, ServiceOptype::SERVICE_ONLINE);
            }
            // 下线通知
            void offline_Notifi(const std::string &method, const Address &host)
            {
                return notify(method, host, ServiceOptype::SERVICE_OFFLINE);
            }

        private:
            void notify(const std::string &method, const Address &host, ServiceOptype type)
            {
                std::unique_lock<std::mutex>(__mutex);
                auto it = _discover.find(method);
                if (it == _discover.end())
                {
                    return;
                }
                auto &discover_set = it->second;
                auto service_msg = MessageFactory::create<ServiceRequest>();
                service_msg->setId(RPC::UUID::uuid());
                service_msg->setMtype(Mtype::REQ_SERVICE);
                service_msg->setServiceMethod(method);
                service_msg->setHost(host);
                service_msg->setServicetype(type);
                for (auto &its : discover_set)
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
            public:
            using ptr = std::shared_ptr<PDManager>;
            void OnServiceRequest(const BaseConnection::ptr &conn, const ServiceRequest::ptr &req)
            {
                // 服务端就只会收到两种请求：1、服务发现 2、服务注册
                auto type = req->optype();
                if (type == ServiceOptype::SERVICE_REGISTRY)
                {
                    _proc->addProvider(conn, req->Host(), req->method());                    
                    _disc->online_Notifi(req->method(), req->Host());
                    return RegistryResponse(conn,req);
                    
                }
                else if (type == ServiceOptype::SERVICE_DISCOVERY)
                {
                    _disc->addDiscover(conn, req->method());
                    return DiscoverResponse(conn,req);
                }
                else
                {
                    DLOG("请求成功，但没有对应的服务类型");
                    return ErrorResponse(conn,req);
                }
            }
            void Shutdown(const BaseConnection::ptr &conn)
            {
                auto it = _proc->getProvider(conn);
                
                if (it != ProviderManager::Provider::ptr())
                {
                    // 有这项服务的提供者，将服务提供者提供下线消息通知发现者
                    for (auto &method : it->methods)
                    {
                        _disc->offline_Notifi(method, it->host);
                    }
                    _proc->delProvider(conn);
                }
                _disc->delDiscover(conn);//
            }
            PDManager()
                : _disc(std::make_shared<DiscoverManager>()), _proc(std::make_shared<ProviderManager>())
            {
            }
        private:

            void ErrorResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &req)
            {
                auto service_msg = MessageFactory::create<ServiceResponse>();
                service_msg->setId(req->id());
                service_msg->setMtype(Mtype::RSP_SERVICE);
                service_msg->setServiceMethod(req->method());
                service_msg->setRCode(RCode::RCODE_INVAILD_OPTYPE);
                service_msg->setServicetype(ServiceOptype::SERVICE_UNKNOW);
                return conn->send(service_msg);

            }
            void RegistryResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &req)
            {
                auto service_msg = MessageFactory::create<ServiceResponse>();
                service_msg->setId(req->id());
                service_msg->setMtype(Mtype::RSP_SERVICE);
                service_msg->setServiceMethod(req->method());
                service_msg->setRCode(RCode::RCODE_OK);
                service_msg->setServicetype(ServiceOptype::SERVICE_REGISTRY);
                return conn->send(service_msg);
            }
            void DiscoverResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &req)
            {
                auto service_msg = MessageFactory::create<ServiceResponse>();
                service_msg->setId(req->id());
                service_msg->setServicetype(ServiceOptype::SERVICE_DISCOVERY);
                service_msg->setMtype(Mtype::RSP_SERVICE);
                service_msg->setServiceMethod(req->method());
                auto hosts = _proc->GetAddress(req->method());
                if(hosts.empty())
                {
                    service_msg->setRCode(RCode::RCODE_NOT_FOUND_SERVICE);
                    DLOG("找不到对应服务");
                    return conn->send(service_msg);
                }
                service_msg->setRCode(RCode::RCODE_OK);
                service_msg->setHost(hosts);
                return conn->send(service_msg);

            }

        private:
            DiscoverManager::ptr _disc;
            ProviderManager::ptr _proc;
        };
    }
}
