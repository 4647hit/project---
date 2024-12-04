#include "net.hpp"
#include "Log.hpp"
#include "abstract.hpp"
#include <iostream>
#include <unordered_map>

namespace RPC
{
    class Dispather
    {
    public:
        using ptr = std::shared_ptr<Dispather>;
        bool registerhandle(Mtype type, const MessageCallBack &cb)
        {
            auto it = _handle.find(type);
            if (it == _handle.end())
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _handle.insert(std::make_pair(type, cb));
                return true;
            }
            else
            {
                ILOG("方法已经存在");
                return false;
            }
        }
        void OnMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _handle.find(msg->mtype());
            if (it == _handle.end())
            {
                ELOG("未知的连接类型");
                conn->shutdown();
            }
            else
            {
                it->second(conn, msg);
            }
        }

    private:
        std::mutex _mutex;
        std::unordered_map<Mtype, MessageCallBack> _handle;
    };
}