#pragma once
#include "../client/rpc_client.hpp"
#include "../common/Dispather.hpp"
#include "rpc_registry.hpp"
#include "rpc_route.hpp"
#include "rpc_topic_server.hpp"

namespace RPC
{
    namespace Server
    {
        class RegisterCenter
        {
            public:
            using ptr = std::shared_ptr<RegisterCenter>;
            RegisterCenter(int port): _pd_manager(std::make_shared<PDManager>()),_dispather(std::make_shared<Dispather>())
            {
                auto cb = std::bind(&PDManager::OnServiceRequest, _pd_manager.get(), std::placeholders::_1,std::placeholders::_2);
                _dispather->registerhandle<RPC::ServiceRequest>(Mtype::REQ_SERVICE, cb);

                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(),
                 std::placeholders::_1, std::placeholders::_2);
             
                _server = RPC::ServerFactory::create(port);
                _server->setMessageCallBack(message_cb);

                auto ncb = std::bind(&RegisterCenter::CloseCallback,this,std::placeholders::_1);
                _server->setCloseCallBack(ncb);
            }
            void CloseCallback(const BaseConnection::ptr& conn)
            {
                return _pd_manager->Shutdown(conn);
            }
            void Start()
            {
                _server->start();
            }
            private:
            PDManager::ptr _pd_manager;
            Dispather::ptr _dispather;
            BaseServer::ptr _server;
        };


        class Route_Server
        {
            public:
            using ptr = std::shared_ptr<Route_Server>;

            //一个是对外提供rpc服务的地址/，还有一个是服务中心的服务端地址，用于服务启用后连接注册中心的注册服务时使用
            Route_Server(const Address& host,bool enableRegister = false, const Address& register_center_host = Address()):
            _host(host),
            _enableRegistry(enableRegister),
            _router(std::make_shared<RpcRouter>()),
            _dispather(std::make_shared<Dispather>())
            {
                auto cb = std::bind(&RPC::Server::RpcRouter::OnRpcRequest,_router.get(),
                std::placeholders::_1,std::placeholders::_2);

                // auto dispather = std::make_shared<Dispather>();
                _dispather->registerhandle<RPC::RpcRequest>(Mtype::REQ_RPC, cb);
                
                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(),
                 std::placeholders::_1, std::placeholders::_2);
                 DLOG("------------------------------------");
                _server = RPC::ServerFactory::create(host.second);
                _server->setMessageCallBack(message_cb);
                if(_enableRegistry)
                {
                    printf("enble\n");
                   _req_client =  std::make_shared<Client::ClientProvider>(
                    register_center_host.first,register_center_host.second);
                }
            }
            void Start()
            {
                _server->start();
            }

            void RegisterMethod(const ServerDescribe::ptr& service)
            {
                if(_enableRegistry)
                {
                    _req_client->RegistryMethod(_host,service->Methodname());//
                }
                _router->registerMethod(service);
            }
            private:
            Address _host;//对外提供服务的地址
            bool _enableRegistry;
            Client::ClientProvider::ptr _req_client;
            RpcRouter::ptr _router;
            Dispather::ptr _dispather;
            BaseServer::ptr _server;      
        };


        class TopicCenter
        {
            public:
            using ptr = std::shared_ptr<TopicCenter>;
            TopicCenter(int port): _topic_manager(std::make_shared<TopicManager>()),_dispather(std::make_shared<Dispather>())
            {
                auto cb = std::bind(&TopicManager::OnTopicRequest, _topic_manager.get(), std::placeholders::_1,std::placeholders::_2);
                _dispather->registerhandle<RPC::TopicRequest>(Mtype::REQ_TOPIC, cb);

                auto message_cb = std::bind(&Dispather::OnMessage, _dispather.get(),
                 std::placeholders::_1, std::placeholders::_2);
             
                _server = RPC::ServerFactory::create(port);
                _server->setMessageCallBack(message_cb);

                auto ncb = std::bind(&TopicCenter::CloseCallback,this,std::placeholders::_1);
                _server->setCloseCallBack(ncb);
            }
            void CloseCallback(const BaseConnection::ptr& conn)
            {
                return _topic_manager->OnShutdown(conn);
            }
            void Start()
            {
                _server->start();
            }
            private:
            RPC::Server::TopicManager::ptr _topic_manager;
            Dispather::ptr _dispather;
            BaseServer::ptr _server;
        };

        
    }
    
}



