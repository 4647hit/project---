#include "request.hpp"
#include "iostream"

namespace RPC
{
    namespace Client
    {

        class Provider
        {
            using ptr = std::shared_ptr<Provider>;
            Provider(Requestor::ptr reqs) : request(reqs)
            {
            }
            bool RegistryMethod(const BaseConnection::ptr conn, const Address &host, const std::string method)
            {
                auto service_msg = MessageFactory::create<ServiceRequest>();
                service_msg->setId(RPC::UUID::uuid());
                service_msg->setMtype(Mtype::REQ_SERVICE);
                service_msg->setServiceMethod(method);
                service_msg->setHost(host);
                service_msg->setServicetype(ServiceOptype::SERVICE_REGISTRY);

                auto service_rsp = MessageFactory::create<BaseMessage>();
                bool ret = request->send(conn, service_msg, service_rsp);
                if (ret == false)
                {
                    DLOG("服务注册失败");
                    return false;
                }
                auto rsp = std::dynamic_pointer_cast<ServiceResponse>(service_rsp);
                if (rsp.get() == nullptr)
                {
                    DLOG("消息类型向下转换失败");
                    return false;
                }

                if (rsp->rcode() != RCode::RCODE_OK)
                {
                    DLOG("注册失败的原因 %s", RPC::Rcode_Reson(rsp->rcode()));
                    return false;
                }
                return true;
            }

        private:
            Requestor::ptr request;
        };
        class Method_Host
        {
        public:
            using ptr = std::shared_ptr<Method_Host>;
            Method_Host() : _index(0) {}
            Method_Host(const std::vector<Address> &host) : _host(host.begin(), host.end()), _index(0) {}
            // 中途上线或下线时调用的相关接口
            void append_host(const Address &h)
            {
                std::unique_lock<std::mutex>(_mutex);
                _host.emplace_back(h);
            }
            void remove_host(const Address &h)
            {
                std::unique_lock<std::mutex>(_mutex);
                for (auto it = _host.begin(); it < _host.end(); it++)
                {
                    if (*it == h)
                    {
                        _host.erase(it);
                    }
                }
            }
            Address chose_host()
            {
                std::unique_lock<std::mutex>(_mutex);
                return _host[_index++ % _host.size()];
            }
            bool empty()
            {
                std::unique_lock<std::mutex>(_mutex);
                return _host.empty();
            }

        private:
            std::mutex _mutex;
            size_t _index;
            std::vector<Address> _host;
            std::string method;
        };
        class Discover
        {
        public:
            // 准备发现服务,返回提供服务者的ip地址信息
            void DiscoverService(const BaseConnection::ptr &conn, const std::string &method, Address &host)
            {
                {
                    std::unique_lock<std::mutex>(_mutex);
                    auto it = _methods_host.find(method);
                    if (it != _methods_host.end())
                    {
                        if (it->second->empty())
                        {
                            host = it->second->chose_host();
                        }
                    }
                }
                // 没有对应的主机提供服务,发送请求客户端，等待客户端的回复响应

                auto service_msg = MessageFactory::create<ServiceRequest>();
                service_msg->setId(RPC::UUID::uuid());
                service_msg->setMtype(Mtype::REQ_SERVICE);
                service_msg->setServiceMethod(method);
                service_msg->setServicetype(ServiceOptype::SERVICE_DISCOVERY);

                auto host_rsp = MessageFactory::create<BaseMessage>();
                bool ret = request->send(conn, service_msg, host_rsp);
                if (ret == false)
                {
                    DLOG("请求发送失败");
                    return;
                }

                auto rsp = std::dynamic_pointer_cast<ServiceResponse>(host_rsp);
                if (rsp.get() == nullptr)
                {
                    DLOG("指针类型转换失败");
                    return;
                }

                if (rsp->rcode() != RCode::RCODE_OK)
                {
                    DLOG("注册失败原因，%s", Rcode_Reson(rsp->rcode()));
                    return;
                }
                // 有新的提供者地址通过响应发送过来了
                auto host_ptr = std::make_shared<Method_Host>();
                if (rsp->Host().empty())
                {
                    DLOG("未有提供者");
                    return;
                }
                for (auto new_host : rsp->Host())
                {
                    host_ptr->append_host(new_host);
                }
                _methods_host[method] = host_ptr;
                host = host_ptr->chose_host();
            }
            // 上线或下线服务时，Discover的处理回调；
            void OnServiceRequest(const BaseConnection &conn, const ServiceRequest::ptr &msg)
            {
                auto service_type = msg->optype();
                // 上线
                if (service_type == ServiceOptype::SERVICE_ONLINE)
                {
                    if (_methods_host[msg->method()] != nullptr)
                    {
                        _methods_host[msg->method()]->append_host(msg->Host());
                    }
                    else
                    {
                        auto new_host = std::make_shared<Method_Host>();
                        new_host->append_host(msg->Host());
                        _methods_host[msg->method()] = new_host;
                    }
                }
                // 下线
                if (service_type == ServiceOptype::SERVICE_OFFLINE)
                {
                    auto it = _methods_host[msg->method()];
                    if (!it->empty())
                    {
                        it->remove_host(msg->Host());
                    }
                    else
                    {
                        return;
                    }
                }
            }

        private:
            std::unordered_map<std::string, Method_Host::ptr> _methods_host; // 提供方法对应的主机ip
            std::mutex _mutex;
            Requestor::ptr request;
        };
    }
}
