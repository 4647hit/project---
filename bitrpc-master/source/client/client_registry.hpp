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
            Method_Host()
            {
            }
            void append(const Address &h);
            Address chose_host();

        private:
            std::mutex _mutex;
            size_t _index;
            std::vector<Address> _host;
            std::string method;
        };
        class Discover
        {
            public:
            //发现服务
            void DiscoverService(std::string method,const Address& host);
            //上线或下线服务时，Discover的处理回调；
            void OnServiceRequest(const BaseConnection& conn, const ServiceRequest::ptr& msg);
            private:
            std::unordered_map<std::string,Method_Host::ptr> _methods_host;
            std::mutex _mutex;
            Requestor::ptr request;
        };
    }
}
