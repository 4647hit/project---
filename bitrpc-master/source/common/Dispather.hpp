#pragma once
#include "net.hpp"
#include "abstract.hpp"
#include "message.hpp"
#include <iostream>
#include <unordered_map>
#include <memory>
#include <functional>
namespace RPC
{
    class Callback
    {
    public:
        using ptr = std::shared_ptr<Callback>;
        virtual void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) = 0;
    };
    template <typename T>
    class CallbackT : public Callback
    {
    public:
        using MessageCb = std::function<void(const BaseConnection::ptr &conn, std::shared_ptr<T> &msg)>;
        using ptr = std::shared_ptr<CallbackT<T>>;
    public:
        CallbackT(const MessageCb &_cb) : _handle(_cb)
        {}
        void onMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg) override
        {
            auto it = std::dynamic_pointer_cast<T>(msg);
            _handle(conn, it);
        }

    private:
        MessageCb _handle;
    };
    class Dispather
    {
    public:
        using ptr = std::shared_ptr<Dispather>;
        template <typename T>
        void registerhandle(Mtype type, const typename CallbackT<T>::MessageCb &handle)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = std::make_shared<CallbackT<T>>(handle);
            _handle.insert(std::make_pair(type, it));
        }
        void OnMessage(const BaseConnection::ptr &conn, BaseMessage::ptr &msg)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _handle.find(msg->mtype());
            if (it != _handle.end())
            {
                return it->second->onMessage(conn, msg);
            }
            else
            {
                ELOG("未知的连接类型");
                conn->shutdown();
            }
        }

    private:
        std::mutex _mutex;
        std::unordered_map<Mtype,Callback::ptr> _handle;
    };
}