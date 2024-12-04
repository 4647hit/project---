#pragma once
#include <memory>
#include <iostream>
#include <functional>
#include "Message-type.hpp"
using namespace RPC;
namespace RPC
{
    class BaseMessage
    {
    public:
        using ptr = std::shared_ptr<BaseMessage>;
        virtual Mtype mtype() { return _mtype; }
        virtual std::string id() { return _rid; }
        virtual void setMtype(Mtype type)
        {
            _mtype = type;
        }
        virtual void setId(std::string rid)
        {
            _rid = rid;
        }
        virtual std::string serialize() = 0;
        virtual bool unserialize(const std::string &body) = 0;
        virtual bool check() = 0;
    private:
        Mtype _mtype;
        std::string _rid;
    };
    class BaseBuffer
    {
    public:
        using ptr = std::shared_ptr<BaseBuffer>;

        virtual size_t readablesize() = 0;
        virtual int32_t peekInt32() = 0;
        virtual void retrieveInt32() = 0; // 删除32
        virtual int32_t readInt32() = 0;
        virtual std::string retrieveAsString(size_t len) = 0;
    };
    class BaseProtocol
    {
    public:
        using ptr = std::shared_ptr<BaseProtocol>;

    public:
        virtual bool canProcessed(const BaseBuffer::ptr &val) = 0;
        virtual bool OnMessage(const BaseBuffer::ptr &buf, BaseMessage::ptr &msg) = 0;
        virtual std::string serialize(const BaseMessage::ptr &ptr) = 0;
    };
    class BaseConnection
    {
    public:
        using ptr = std::shared_ptr<BaseConnection>;
        virtual void send(const BaseMessage::ptr &msg) = 0;
        virtual void shutdown() = 0;
        virtual bool connected() = 0;
    };
    using ConnectionCallBack = std::function<void(const BaseConnection::ptr &)>;
    using CloseCallBack = std::function<void(const BaseConnection::ptr &)>;
    using MessageCallBack = std::function<void(const BaseConnection::ptr &, BaseMessage::ptr &)>;
    class BaseServer
    {
        public:
        using ptr = std::shared_ptr<BaseServer>;
        virtual void setConnectionCallBack(const ConnectionCallBack &cb)
        {
            _cb_connection = cb;
        }
        virtual void setCloseCallBack(const CloseCallBack &cb)
        {
            _cb_close = cb;
        }
        virtual void setMessageCallBack(const MessageCallBack &cb)
        {
            _cb_message = cb;
        }
        virtual void start() = 0;
    protected:
        ConnectionCallBack _cb_connection;
        CloseCallBack _cb_close;
        MessageCallBack _cb_message;
    };
    class BaseClient
    {
        public:
        using ptr = std::shared_ptr<BaseClient>;
           virtual void setConnectionCallBack(const ConnectionCallBack &cb)
        {
            _cb_connection = cb;
        }
        virtual void setCloseCallBack(const CloseCallBack &cb)
        {
            _cb_close = cb;
        }
        virtual void setMessageCallBack(const MessageCallBack &cb)
        {
            _cb_message = cb;
        }
        virtual void connect() = 0;
        virtual void shutdown() = 0; 
        virtual bool send(const BaseMessage::ptr& val) = 0;
        virtual BaseConnection::ptr connection() = 0;
        virtual bool connected() = 0;
    protected:
        ConnectionCallBack _cb_connection;
        CloseCallBack _cb_close;
        MessageCallBack _cb_message;
    };
}