#include "../common/Log.hpp"
#include "../common/message.hpp"
#include "../common/abstract.hpp"
#include <mutex>

namespace RPC
{
    namespace Server
    {
        enum Valuetype
        {
            BOOL = 0,
            INT,
            FLOAT,
            STRING,
            ARRAY,
            OBJECT,
        };
        class ServerDescribe
        {
        public:
            using ptr = std::shared_ptr<ServerDescribe>;
            using ServiceCallback = std::function<void(const Json::Value &, Json::Value &)>;
            using param = std::pair<std::string, Valuetype>;
            ServerDescribe(Valuetype &&rtype, std::string &&mname, ServiceCallback &&callback, std::vector<param> &&desc) : _return_value(std::move(rtype)),
             method_name(std::move(mname)), _cb(std::move(callback)), param_des(std::move(desc))
            {
            }
            bool checkparam(const Json::Value &params)
            {
                for (auto dest : param_des)
                {
                    if (params.isMember(dest.first) == false)
                    {
                        ELOG("消息字段缺失不完整");
                        return false;
                    }
                    if (check(dest.second, params[dest.first]) == false)
                    {
                        ELOG("参数类型校验失败");
                        return false;
                    }
                }
                return true;
            }
            bool call(const Json::Value& params, Json::Value& result)
            {
                _cb(params, result);
                if (rtypecheck(result) == false)
                {
                    ELOG("回调函数中的类型校验失败");
                    return false;
                }
                return true;
            }
            const std::string &Methodname()
            {
                return method_name;
            }

        private:
            Valuetype _return_value;      // 返回值的参数类型
            std::string method_name;      // 方法名称
            ServiceCallback _cb;          // 实际的业务回调
            std::vector<param> param_des; // 参数字段的描述
        private:
            bool rtypecheck(const Json::Value &val)
            {
                return check(_return_value, val);
            }
            bool check(Valuetype rtype, const Json::Value &val)
            {
                switch (rtype)
                {
                case Valuetype::BOOL:
                    return val.isBool();
                case Valuetype::ARRAY:
                    return val.isArray();
                case Valuetype::FLOAT:
                    return val.isNumeric();
                case Valuetype::INT:
                    return val.isIntegral();
                case Valuetype::OBJECT:
                    return val.isObject();
                case Valuetype::STRING:
                    return val.isString();
                default:
                    break;
                }
                return false;
            }
        };
        // 建造者模式
        class SDescribeFactory
        {
            using ServiceCallback = std::function<void(const Json::Value &, Json::Value &)>;

        public:
            void SetreturnValue(const Valuetype &type)
            {
                _return_value = type;
            }
            void SetMethodname(const std::string &name)
            {
                method_name = name;
            }
            void SetServerCallback(const ServerDescribe::ServiceCallback &cb)
            {
                _cb = cb;
            }
            void Setparams(const std::string &name, Valuetype vtype)
            {
                param_des.push_back(ServerDescribe::param(name, vtype));
            }
            ServerDescribe::ptr build()
            {
                return std::make_shared<ServerDescribe>(std::move(_return_value), std::move(method_name),std::move( _cb), std::move(param_des));
            }

        private:
            Valuetype _return_value;                      // 返回值的参数类型
            std::string method_name;                      // 方法名称
            ServiceCallback _cb;                          // 实际的业务回调
            std::vector<ServerDescribe::param> param_des; // 参数字段的描述
        };
        class ServerManager
        {
        public:
            using ptr = std::shared_ptr<ServerManager>;
            void insert(const ServerDescribe::ptr &dest)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                if ((_services.find(dest->Methodname()) == _services.end()))
                {
                    _services.insert(make_pair(dest->Methodname(), dest));
                }
            }
            ServerDescribe::ptr select(const std::string &Mname)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _services.find(Mname);
                if (it == nullptr)
                {
                    return ServerDescribe::ptr();
                }
                return it->second;
            }
            void remove(const std::string &name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _services.erase(name);
            }

        private:
            std::mutex _mutex;
            std::unordered_map<std::string, ServerDescribe::ptr> _services;
        };
        class ServerDescribeFactory // 工厂
        {
        public:
            static ServerDescribe::ptr create();
        };
        class RpcRouter
        {
        public:
            using ptr = std::shared_ptr<RpcRouter>;
            RpcRouter() : server_manger(std::make_shared<ServerManager>())
            {
            }
            //
            void OnRpcRequest(const BaseConnection::ptr &conn, RPC::RpcRequest::ptr &request)
            {
                // 检查是否存在服务
                auto service = server_manger->select(request->method());
                if (service.get() == nullptr)
                {
                    ELOG("%s 未发现服务", request->method().c_str());
                    return response(conn, request, Json::Value(), RCode::RCODE_NOT_FOUND_SERVICE);
                }
                // check params
                if (service->checkparam(request->params()) == false)
                {
                    ELOG("参数校验失败");
                    return response(conn, request, Json::Value(), RCode::RCODE_INVALID_PARAMS);
                }
                // execute callback function
                Json::Value result;
                if (service->call(request->params(), result) == false)
                {
                    ELOG("内部参数错误");
                    return response(conn, request, Json::Value(), RCode::RCODE_INTERNAL_ERROR);
                }
                printf("send response over\n");
                return response(conn, request, result, RCode::RCODE_OK);
            }
            void registerMethod(const ServerDescribe::ptr &des)
            {
                return server_manger->insert(des);
            }

        private:
            void response(const BaseConnection::ptr &conn, RPC::RpcRequest::ptr &request, const Json::Value &res, RCode rcode)
            {
                auto msg = MessageFactory::create<RpcResponse>();
                msg->setId(request->id());
                msg->setMtype(Mtype::RSP_RPC);
                msg->setRCode(rcode);
                msg->setResult(res);
                conn->send(msg);

            }
            ServerManager::ptr server_manger;
        };

    }
}
