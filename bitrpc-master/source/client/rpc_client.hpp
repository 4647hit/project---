#pragma once
#include "../common/Log.hpp"
#include "../common/message.hpp"
#include "../common/abstract.hpp"
#include "request.hpp"
#include "../common/Dispather.hpp"
#include "client_registry.hpp"
#include "rpc-caller.hpp"

namespace RPC
{
    namespace Client
    {
        class ClientProvider
        {
            using ptr = std::shared_ptr<ClientProvider>;

            // 提供注册中心的ip和地址
            ClientProvider(const std::string &ip, int port)
                : _request(std::make_shared<Requestor>()),
                  _dispather(std::make_shared<Dispather>()),
                  _provider(std::make_shared<Provider>(_request))
            {
                _client = RPC::ClientFactory::create("ip", port);
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

        class ClientDiscover
        {
        public:
            using ptr = std::shared_ptr<ClientDiscover>;
            //
            ClientDiscover(const std::string &ip, int port) : _request(std::make_shared<Requestor>()),
                                                              _dispather(std::make_shared<Dispather>()),
                                                              _discover(std::make_shared<Discover>(_request))
            {

                auto rsp_cb = std::bind(&Client::Requestor::onResponse, _request.get(), std::placeholders::_1, std::placeholders::_2);
                _dispather->registerhandle<BaseMessage>(Mtype::RSP_SERVICE, rsp_cb);

                auto req_cb = std::bind(&Client::Discover::OnServiceRequest, _discover.get(), std::placeholders::_1, std::placeholders::_2);
                _dispather->registerhandle<ServiceRequest>(Mtype::REQ_SERVICE,req_cb);//xianzheyang ， 这里存在问题

                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(), std::placeholders::_1, std::placeholders::_2);

                _client = RPC::ClientFactory::create("ip", port);
                _client->setMessageCallBack(message_cb);
                _client->connect();
            }
            // 返回提供服务的服务主机地址
            void DiscoverService(const std::string &method, Address &host)
            {
                return _discover->DiscoverService(_client->connection(),method,host);
            }
        private:
            Requestor::ptr _request;
            BaseClient::ptr _client;
            Dispather::ptr _dispather;
            Client::Discover::ptr _discover;
        };
        class RpcClient_Service
        {
            using ptr = std::shared_ptr<RpcClient_Service>;
            // 如果开启发现服务，则初始化的地址就为注册中心的地址。如果不是，那就是发现者的地址
            RpcClient_Service(bool enablediscover, const std::string &ip, int port) : _enablediscover(enablediscover)
            {
            }
            bool call(const BaseConnection::ptr &conn, const std::string &method_name, const Json::Value &params, Json::Value &result);
            bool call(const BaseConnection::ptr &conn, const std::string &method_name, const Json::Value &params, RpcCaller::JsonAsyncResponse &result);
            bool call(const BaseConnection::ptr &conn, const std::string &method_name, const Json::Value &params, const RpcCaller::JsonResponseCallback &cb);

        private:
            bool _enablediscover;
            BaseClient::ptr _client;
            Requestor::ptr _request;
            Dispather::ptr _dispather;
            RpcCaller::ptr _caller;
            ClientDiscover::ptr _discover;
        };
    }
}