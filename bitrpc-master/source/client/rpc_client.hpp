#pragma once
#include "../common/Log.hpp"
#include "../common/message.hpp"
#include "../common/abstract.hpp"
#include "request.hpp"
#include "../common/Dispather.hpp"
#include "client_registry.hpp"
#include "rpc-caller.hpp"
#include "rpc_topic.hpp"

namespace RPC
{
    namespace Client
    {
        class ClientProvider // registerclient 客户端注册封装
        {
        public:
            using ptr = std::shared_ptr<ClientProvider>;

            // 需要提供注册中心的ip和地址，进行绑定连接
            ClientProvider(const std::string &ip, int port)
                : _request(std::make_shared<Requestor>()),
                  _dispather(std::make_shared<Dispather>()),
                  _provider(std::make_shared<Provider>(_request))
            {
                _client = RPC::ClientFactory::create(ip, port);
                auto rsp_cb = std::bind(&Client::Requestor::onResponse, _request.get(), std::placeholders::_1, std::placeholders::_2);

                _dispather->registerhandle<RPC::BaseMessage>(Mtype::RSP_SERVICE, rsp_cb);
                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(), std::placeholders::_1, std::placeholders::_2);

                _client->setMessageCallBack(message_cb);
                _client->connect();
            }
            bool RegistryMethod(const Address &host, const std::string method)
            {
                return _provider->RegistryMethod(_client->connection(), host, method);
            }

        private:
            Requestor::ptr _request;
            BaseClient::ptr _client;
            Dispather::ptr _dispather;
            Client::Provider::ptr _provider;
        };

        class ClientDiscover // 客户端发现封装
        {
        public:
            using ptr = std::shared_ptr<ClientDiscover>;
            using OfflineCallback = std::function<void(const Address &host)>;
            // 注册中心的地址
            ClientDiscover(const std::string &ip, int port, const OfflineCallback &cb) : _request(std::make_shared<Requestor>()),
                                                                                         _dispather(std::make_shared<Dispather>()),
                                                                                         _discover(std::make_shared<Discover>(cb, _request))
            {

                auto rsp_cb = std::bind(&Client::Requestor::onResponse, _request.get(), std::placeholders::_1, std::placeholders::_2);
                _dispather->registerhandle<BaseMessage>(Mtype::RSP_SERVICE, rsp_cb);

                auto req_cb = std::bind(&Client::Discover::OnServiceRequest, _discover.get(), std::placeholders::_1, std::placeholders::_2);
                _dispather->registerhandle<ServiceRequest>(Mtype::REQ_SERVICE, req_cb); //

                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(), std::placeholders::_1, std::placeholders::_2);

                _client = RPC::ClientFactory::create(ip, port);
                _client->setMessageCallBack(message_cb);
                _client->connect();
            }
            // 返回提供服务的服务主机地址
            bool DiscoverService(const std::string &method, Address &host)
            {
                return _discover->DiscoverService(_client->connection(), method, host);
            }

        private:
            Requestor::ptr _request;
            BaseClient::ptr _client;
            Dispather::ptr _dispather;
            Client::Discover::ptr _discover;
        };
        class RpcClient
        {
        public:
            using ptr = std::shared_ptr<RpcClient>;
            // 如果开启发现服务，则初始化的地址就为注册中心的地址。如果不是，那就是发现者的地址
            RpcClient(bool enablediscover, const std::string &ip, int port) : _enablediscover(enablediscover),
                                                                              _request(std::make_shared<Requestor>()),
                                                                              _dispather(std::make_shared<Dispather>()),
                                                                              _caller(std::make_shared<RpcCaller>(_request))
            {
                DLOG("-------------------------------------------client ");
                // rpc请求对应的回调处理
                auto rsp_cb = std::bind(&Client::Requestor::onResponse, _request.get(), std::placeholders::_1, std::placeholders::_2);
                _dispather->registerhandle<BaseMessage>(Mtype::RSP_RPC, rsp_cb);
                // 如果是发现服务，那么传入的就是注册中心地址，如果不是，传入地址就为的提供者地址

                if (_enablediscover)
                {
                    auto offlinecallback = std::bind(&RpcClient::delClient, this, std::placeholders::_1);
                    _discover = std::make_shared<ClientDiscover>(ip, port, offlinecallback);
                }
                else
                {
                    auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(), std::placeholders::_1, std::placeholders::_2);
                    _client = RPC::ClientFactory::create(ip, port);
                    _client->setMessageCallBack(message_cb);
                    _client->connect();
                }
            }
            bool call(const std::string &method_name, const Json::Value &params, Json::Value &result)
            {
                DLOG("------------------------------------");
                auto ptr = getClient(method_name);
                if (ptr.get() == nullptr)
                {
                    DLOG("查询服务端地址失败");
                    return false;
                }
                DLOG("------------------------------------");
                return _caller->call(ptr->connection(), method_name, params, result);
            }
            bool call(const std::string &method_name, const Json::Value &params, RpcCaller::JsonAsyncResponse &result)
            {
                auto ptr = getClient(method_name);
                if (ptr.get() == nullptr)
                {
                    DLOG("查询服务端地址失败");
                    return false;
                }
                return _caller->call(ptr->connection(), method_name, params, result);
            }
            bool call(const std::string &method_name, const Json::Value &params, const RpcCaller::JsonResponseCallback &cb)
            {
                {
                    auto ptr = getClient(method_name);
                    if (ptr.get() == nullptr)
                    {
                        DLOG("查询服务端地址失败");
                        return false;
                    }
                    return _caller->call(ptr->connection(), method_name, params, cb);
                }
            }

        private:
            BaseClient::ptr NewClient(const Address &host)
            {
                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(), std::placeholders::_1, std::placeholders::_2);
                auto client = RPC::ClientFactory::create(host.first, host.second);
                client->setMessageCallBack(message_cb);
                client->connect();
                addClient(host, client);
                return client;
            }
            BaseClient::ptr getClient(const Address &host)
            {
                std::unique_lock<std::mutex>(_mutex);
                auto it = _rpc_clients.find(host);
                if (it == _rpc_clients.end())
                {
                    return BaseClient::ptr();
                }
                return it->second;
            }
            BaseClient::ptr getClient(std::string method)
            {
                std::unique_lock<std::mutex>(_mutex);
                Address host;
                BaseClient::ptr ptr;
                if (_enablediscover)
                {
                    // 查询提供者地址信息
                    bool ret = _discover->DiscoverService(method, host);
                    if (ret == false)
                    {
                        DLOG("请求 %s 服务失败", method.c_str());
                    }
                    auto client = getClient(host);
                    if (client.get())
                    {
                        ptr = client;
                    }
                    // 没有查到服务端直接创建
                    else
                    {
                        DLOG("创建客户端成功");
                        ptr = NewClient(host);
                    }
                }
                else
                {
                    ptr = _client;
                }
                return ptr;
            }
            void addClient(const Address &host, const BaseClient::ptr &ptr)
            {
                std::unique_lock<std::mutex>(_mutex);
                _rpc_clients.insert(std::make_pair(host, ptr));
            }
            void delClient(const Address &host)
            {
                std::unique_lock<std::mutex>(_mutex);
                _rpc_clients.erase(host);
            }

        private:
            struct myhash
            {
                size_t operator()(const Address &host) const
                {
                    std::string addr = host.first + std::to_string(host.second);
                    return std::hash<std::string>{}(addr);
                }
            };
            bool _enablediscover;
            std::mutex _mutex;
            Requestor::ptr _request;
            BaseClient::ptr _client; // 未发现的服务客户端
            Dispather::ptr _dispather;
            RpcCaller::ptr _caller;
            ClientDiscover::ptr _discover;
            std::unordered_map<Address, BaseClient::ptr, myhash> _rpc_clients; // 已经发现的提供者服务客户端
        };

        class TopicClient
        {
        public:
            using ptr = std::shared_ptr<TopicClient>;
            using SubCallBack = std::function<void(const std::string &topic_name, const std::string &msg)>;

            TopicClient(const std::string &ip, int port) : _request(std::make_shared<Requestor>()),
                                                           _dispather(std::make_shared<Dispather>()),
                                                           _topic_mg(std::make_shared<TopicManager>(_request))
            {
                // rpc请求对应的回调处理
                auto rsp_cb = std::bind(&Client::Requestor::onResponse, _request.get(), std::placeholders::_1, std::placeholders::_2);
                _dispather->registerhandle<BaseMessage>(Mtype::RSP_TOPIC, rsp_cb);
                // 传入的就是注册中心地址
                auto req_cb = std::bind(&Client::TopicManager::OnPublish, _topic_mg.get(), std::placeholders::_1, std::placeholders::_2);
                _dispather->registerhandle<TopicRequest>(Mtype::REQ_TOPIC, req_cb);

                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(), std::placeholders::_1, std::placeholders::_2);
                _client = RPC::ClientFactory::create(ip, port);
                _client->setMessageCallBack(message_cb);
                _client->connect();
            }
            bool TopicCreate(const std::string &name)
            {
                return _topic_mg->TopicCreate(_client->connection(), name);
            }
            bool TopicRemove(const std::string &name)
            {
                return _topic_mg->TopicRemove(_client->connection(), name);
            }
            bool Subscribe(const BaseConnection::ptr &conn, const std::string &name, const SubCallBack &cb)
            {
                return _topic_mg->Subscribe(_client->connection(), name, cb);
            }
            bool UnSubscribe(const std::string &name)
            {
                return _topic_mg->UnSubscribe(_client->connection(), name);
            }
            bool OnPublishMessage(const std::string &name, const std::string &msg)
            {
                return _topic_mg->OnPublishMessage(_client->connection(), name, msg);
            }
            void Publish(const BaseConnection::ptr &conn, const TopicRequest::ptr &msg)
            {
                return _topic_mg->OnPublish(conn, msg);
            }
            void Shutdown()
            {
                return _client->shutdown();
            }

        private:
        private:
            std::mutex _mutex;
            TopicManager::ptr _topic_mg;
            Requestor::ptr _request;
            BaseClient::ptr _client; // 未发现的服务客户端
            Dispather::ptr _dispather;
        };
    }
}