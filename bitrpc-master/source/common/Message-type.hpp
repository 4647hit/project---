#pragma once
#include <string>
#include <unordered_map>
namespace RPC
{
    #define KEY_METHOD "method"
    #define KEY_PARAMS "parameters"
    #define KEY_TOPIC_KEY "topic_key"
    #define KEY_TOPIC_MEG "topic_msg"
    #define KEY_OPTYPE "optype"
    #define KEY_HOST "host"
    #define KEY_HOST_IP "ip"
    #define KEY_HOST_PORT "port"
    #define KEY_RCODE "rcode"
    #define KEY_RESULT "result"

    enum class Mtype//消息类型
    {
        REQ_RPC = 0,
        RSP_RPC,
        REQ_TOPIC,
        RSP_TOPIC,
        REQ_SERVICE,
        RSP_SERVICE
    };
    //响应状态码
    enum class RCode
    {
        RCODE_OK = 0,
        RCODE_PARSE_FAILED,
        RCODE_INVALID_MGD,
        RCODE_DISCONNECTION,
        RCODE_INVALID_PARAMS,
        RCODE_NOT_FOUND_SERVICE,
        RCODE_INVAILD_OPTYPE,
        RCODE_NOT_FOUND_TOPIC,
        RCODE_INTERNAL_ERROR
    };
    static std::string Rcode_Reson(RCode code)
    {
        static std::unordered_map<RCode,std::string> errR = {
            {RCode::RCODE_OK,"消息成功处理"},
            {RCode::RCODE_PARSE_FAILED,"消息解析失败"},
            {RCode::RCODE_INVALID_MGD,"无效消息"},
            {RCode::RCODE_DISCONNECTION,"连接断开"},
            {RCode::RCODE_INVALID_PARAMS,"无效参数"},
            {RCode::RCODE_NOT_FOUND_SERVICE,"没有找到对应的服务"},
            {RCode::RCODE_INVAILD_OPTYPE,"无效的操作类型"},
            {RCode::RCODE_NOT_FOUND_TOPIC,"没有找到对应的主题"},
            {RCode::RCODE_INTERNAL_ERROR,"内部错误"}
        };
        auto it = errR.find(code);
        if(it == errR.end())
        {
            return "未知错误";
        }
        return it->second;
    }
    enum class RType
    {
        REQ_SYNC = 0,
        REQ_ASYNC,
        REQ_CALLBACK
    };
    enum class TopicOptype
    {
        TOPIC_CREATE = 0,
        TOPIC_REMOVE,
        TOPIC_SUBCRIBE,
        TOPIC_CANCEL,
        TOPIC_PUBLISH
    };
    enum class ServiceOptype
    {
        SERVICE_REGISTRY = 0,
        SERVICE_DISCOVERY,
        SERVICE_ONLINE,
        SERVICE_OFFLINE,
        SERVICE_UNKNOW
    };
}