#include <memory>
#include <string>
#include <iostream>

#include "Log.hpp"
#include "Message-type.hpp"
#include "abstract.hpp"

namespace RPC
{
    class JsonMessage : public BaseMessage
    {
    public:
        using ptr = std::shared_ptr<JsonMessage>;
        virtual std::string serialize() override
        {
            std::string body;
            bool ret = JsonTool::serialize(_body, body);
            if (ret)
            {
                return body;
            }
            return std::string();
        }
        virtual bool unserialize(const std::string &msg) override
        {
            return JsonTool::unserialize(_body, msg);
        }
        virtual bool check() = 0;

    protected:
        Json::Value _body;
    };
    class JsonRequest : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonRequest>;
    };
    class JsonResponse : public JsonMessage
    {
    public:
        using ptr = std::shared_ptr<JsonResponse>;
        virtual bool check() override
        {
            // 检测状态响应码
            if (_body[KEY_RCODE].isNull())
            {
                ELOG("响应中没有状态码");
                return false;
            }
            if (_body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("响应状态码错误");
                return false;
            }
            return true;
        }
        virtual RCode rcode()
        {
            return (RCode)_body[KEY_RCODE].asInt();
        }
        virtual void setRCode(RCode code)
        {
            _body[KEY_RCODE] = (int)code;
        }
    };
    class RpcRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<RpcRequest>;
        virtual bool check() override
        {
            if (_body[KEY_METHOD].isNull() == true ||
                _body[KEY_METHOD].isString() == false)
            {
                ELOG("PRC请求中没有方法请求或请求类型错误");
                return false;
            }
            if (_body[KEY_PARAMS].isNull() == true)
            {
                ELOG("PRC请求中没有方法请求或请求类型错误");
                return false;
            }
            return true;
        }
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }
        void setMethod(const std::string &method_name)
        {
            _body[KEY_METHOD] = method_name;
        }
        Json::Value params()
        {
            return _body[KEY_PARAMS];
        }
        void setParams(const Json::Value &params)
        {
            _body[KEY_PARAMS] = params;
        }
    };
    class TopicRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<TopicRequest>;
        virtual bool check() override
        {
            if (_body[KEY_TOPIC_KEY].isNull() == true ||
                _body[KEY_TOPIC_KEY].isString() == false)
            {
                ELOG("主题请求中没有方法请求或请求类型错误");
                return false;
            }
            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("主题请求中没有操作请求或操作请求类型错误");
                return false;
            }

            if (_body[KEY_OPTYPE].asInt() == (int)TopicOptype::TOPIC_PUBLISH &&
                    _body[KEY_TOPIC_MEG].isNull() == true ||
                _body[KEY_TOPIC_MEG].isString() == false)
            {
                ELOG("主题消息的发布请求中没有消息或者消息内容类型错误");
                return false;
            }
            return true;
        }
        std::string TopicKey()
        {
            return _body[KEY_TOPIC_KEY].asString();
        }
        void setTopicKey(const std::string &method_name)
        {
            _body[KEY_TOPIC_KEY] = method_name;
        }
        TopicOptype Topictype()
        {
            return (TopicOptype)_body[KEY_OPTYPE].asInt();
        }
        void setTopictype(TopicOptype type_name)
        {
            _body[KEY_OPTYPE] = (int)type_name;
        }
        std::string TopicMsg()
        {
            return _body[KEY_TOPIC_MEG].asString();
        }
        void setTopicMsg(const std::string &message)
        {
            _body[KEY_TOPIC_MEG] = message;
        }
    };
    typedef std::pair<std::string, int> Address;
    class ServiceRequest : public JsonRequest
    {
    public:
        using ptr = std::shared_ptr<ServiceRequest>;
        virtual bool check() override
        {
            if (_body[KEY_METHOD].isNull() == true ||
                _body[KEY_METHOD].isString() == false)
            {
                ELOG("服务请求中没有方法请求或请求类型错误");
                return false;
            }
            if (_body[KEY_OPTYPE].isNull() == true ||
                _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("服务请求中没有方法请求或请求类型错误");
                return false;
            }

            if (_body[KEY_OPTYPE].asInt() != (int)ServiceOptype::SERVICE_DISCOVERY &&
                _body[KEY_HOST].isNull() == true ||
                _body[KEY_HOST].isObject() == false ||
                _body[KEY_HOST][KEY_HOST_IP].isNull() == true ||
                _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
                _body[KEY_HOST][KEY_HOST_PORT].isNull() == true ||
                _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false)
            {
                ELOG("服务请求中主机地址错误");
                return false;
            }
            return true;
        }
        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }
        void setServiceMethod(const std::string &method_name)
        {
            _body[KEY_METHOD] = method_name;
        }
        ServiceOptype optype()
        {
            return (ServiceOptype)_body[KEY_OPTYPE].asInt();
        }
        void setServicetype(ServiceOptype type_name)
        {
            _body[KEY_OPTYPE] = (int)type_name;
        }
        Address Host()
        {
            Address add;
            add.first = _body[KEY_HOST][KEY_HOST_IP].asString();
            add.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
            return add;
        }
        void setHost(const Address &host)
        {
            Json::Value val;
            val[KEY_HOST_IP] = host.first;
            val[KEY_HOST_PORT] = host.second;
            _body[KEY_HOST] = val;
        }
    };

    class RpcResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<RpcResponse>;
        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("响应中没有响应状态码")
                return false;
            }
            if (_body[KEY_RESULT].isNull() == true)
            {
                ELOG("响应没有结果")
                return false;
            }
            return true;
        }

        Json::Value result()
        {
            return _body[KEY_RESULT];
        }
        void setResult(const Json::Value &result)
        {
            _body[KEY_RESULT] = result;
        }
    };
    class TopicResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<TopicResponse>;
    };
    class ServiceResponse : public JsonResponse
    {
    public:
        using ptr = std::shared_ptr<ServiceResponse>;
        virtual bool check() override
        {
            if (_body[KEY_RCODE].isNull() == true || _body[KEY_RCODE].isIntegral() == false)
            {
                ELOG("响应中没有响应状态码")
                return false;
            }
            if (_body[KEY_OPTYPE].isNull() == true || _body[KEY_OPTYPE].isIntegral() == false)
            {
                ELOG("服务请求中没有方法请求或请求类型错误");
                return false;
            }
            if (_body[KEY_OPTYPE].asInt() == (int)ServiceOptype::SERVICE_DISCOVERY &&
                (_body[KEY_METHOD].isString() == false ||
                 _body[KEY_METHOD].isNull() == true ||
                 _body[KEY_HOST].isNull() == true ||
                 _body[KEY_HOST].isArray() == false))
            {
                ELOG("服务发现中出现响应信息字段错误");
                return false;
            }
            return true;
        }

        std::string method()
        {
            return _body[KEY_METHOD].asString();
        }
        void setServiceMethod(const std::string &method_name)
        {
            _body[KEY_METHOD] = method_name;
        }
        ServiceOptype optype()
        {
            return (ServiceOptype)_body[KEY_OPTYPE].asInt();
        }
        void setServicetype(ServiceOptype type_name)
        {
            _body[KEY_OPTYPE] = (int)type_name;
        }
        void setHost(const std::vector<Address> &add)
        {
            // {method : add
            // host:
            // ip1:... port1:....
            // ip2:... port2:....
            for (auto &i : add)
            {
                Json::Value val;
                val[KEY_HOST_IP] = i.first;
                val[KEY_HOST_PORT] = i.second;
                _body[KEY_HOST].append(val);
            }
        }
        std::vector<Address> Host()
        {
            std::vector<Address> hosts;
            int size = _body[KEY_HOST].size();
            for (int i = 0; i < size; i++)
            {
                Address host;
                host.first = _body[KEY_HOST][i][KEY_HOST_IP].asString();
                host.second = _body[KEY_HOST][i][KEY_HOST_PORT].asInt();
                hosts.push_back(host);
            }
            return hosts;
        }
    };

    class MessageFactory
    {
    public:
        static BaseMessage::ptr create(Mtype mtype)
        {
            switch (mtype)
            {
            case Mtype::REQ_RPC:
                return std::make_shared<RpcRequest>();
            case Mtype::RSP_RPC:
                return std::make_shared<RpcResponse>();
            case Mtype::REQ_TOPIC:
                return std::make_shared<TopicRequest>();
            case Mtype::RSP_TOPIC:
                return std::make_shared<TopicResponse>();
            case Mtype::REQ_SERVICE:
                return std::make_shared<ServiceRequest>();
            case Mtype::RSP_SERVICE:
                return std::make_shared<ServiceResponse>();
            }
            return RPC::BaseMessage::ptr();
        }
        template <typename T, typename... Args>
        static std::shared_ptr<T> create(Args &&...args)
        {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
    };
}