#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpClient.h>
#include <iostream>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/EventLoopThread.h>
#include "abstract.hpp"
#include "Log.hpp"
#include "Message-type.hpp"
#include "message.hpp"
#include <unordered_map>
#include <mutex>

namespace RPC
{
    class MudouBuffer : public BaseBuffer
    {
    public:
        MudouBuffer(muduo::net::Buffer *buffer) : _buf(buffer)
        {
        }
        using ptr = std::shared_ptr<MudouBuffer>;
        virtual size_t readablesize()
        {
            return _buf->readableBytes();
        }
        virtual int32_t peekInt32()
        {
            // muduo库是网络库，从缓冲区取出4字节整型会进行网络字节序的转换
            return _buf->peekInt32();
        }
        virtual void retrieveInt32() // 删除32
        {
            return _buf->retrieveInt32();
        }
        virtual int32_t readInt32()
        {
            return _buf->readInt32();
        }
        virtual std::string retrieveAsString(size_t len)
        {
            return _buf->retrieveAsString(len);
        }

    private:
        muduo::net::Buffer *_buf;
    };

    class MudouBufferFactory
    {
    public:
        template <typename... Args>
        static RPC::BaseBuffer::ptr create(Args &&...args)
        {
            return std::make_shared<MudouBuffer>(std::forward<Args>(args)...);
        }
    };
    class LVProtocol : public BaseProtocol
    {
    public:
        //|--len--|--mtype--|--idlen--|--id--|--body--|
        using ptr = std::shared_ptr<LVProtocol>;

    public:
        virtual bool canProcessed(const BaseBuffer::ptr &val)
        {
            if (val->readablesize() < lenFieldslength)
            {
                return false;
            }
            int32_t total_len = val->peekInt32();
            if (val->readablesize() < total_len + lenFieldslength)
            {
                return false;
            }
            return true;
        }
        virtual bool OnMessage(const BaseBuffer::ptr &buf, BaseMessage::ptr &msg) override
        {
            // 解析消息
            //|--len--|--mtype--|--idlen--|--id--|--body--|
            int32_t len = buf->readInt32();
            int32_t mtype = buf->readInt32();
            int32_t idlen = buf->readInt32();

            std::string id = buf->retrieveAsString(idlen);
            int32_t body_len = len - mTypeFieldslength - IdFieldslength - idlen;
            std::string body = buf->retrieveAsString(body_len);

            msg = MessageFactory::create((Mtype)mtype);
            if (msg.get() == nullptr)
            {
                ELOG("消息解析失败")
                return false;
            }
            bool ret = msg->unserialize(body);
            if (ret == false)
            {
                ELOG("反序列化失败")
                return false;
            }
            msg->setId(id);
            msg->setMtype((Mtype)mtype);
            return true;
        }
        virtual std::string serialize(const BaseMessage::ptr &ptr) override
        {
            //|--len--|--mtype--|--idlen--|--id--|--body--|
            std::string body = ptr->serialize();
            std::string id = ptr->id();

            auto mtype = htonl((int32_t)ptr->mtype());
            int32_t mtypelen = htonl(mTypeFieldslength);
            int32_t Idlen = htonl(IdFieldslength);

            std::string result;
            int32_t total_len = mTypeFieldslength + IdFieldslength + body.size() + id.size();
            result.reserve(total_len + lenFieldslength); //?
            int32_t n_total_len = htonl(total_len);
            result.append((char *)&n_total_len, lenFieldslength);
            result.append((char *)&mtype, mTypeFieldslength);
            result.append((char *)&Idlen, IdFieldslength);
            result.append(id);
            result.append(body);

            return result;
        }

    private:
        const size_t lenFieldslength = 4;
        const size_t mTypeFieldslength = 4;
        const size_t IdFieldslength = 4;
    };
    class LVProtovolFactory
    {
    public:
        template <typename... Args>
        static RPC::BaseProtocol::ptr create(Args &&...args)
        {
            return std::make_shared<LVProtocol>(std::forward<Args>(args)...);
        }
    };
    class MuduoConnection : public BaseConnection
    {
    public:
        using ptr = std::shared_ptr<MuduoConnection>;
        MuduoConnection(const BaseProtocol::ptr &protocol, const muduo::net::TcpConnectionPtr &conn) : _protocol(protocol), _conn(conn)
        {
        }
        virtual void send(const BaseMessage::ptr &msg) override
        {
            std::string message = _protocol->serialize(msg);
            if (message.size())
            {
                _conn->send(message);
            }
        }
        virtual void shutdown() override
        {
            _conn->shutdown();
        }
        virtual bool connected() override
        {
            return _conn->connected();
        }

    private:
        BaseProtocol::ptr _protocol;
        muduo::net::TcpConnectionPtr _conn;
    };
    class MuduoConnectionFactory
    {
    public:
        template <typename... Args>
        static RPC::BaseConnection::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
        }
    };

    class MuduoServer : public BaseServer
    {
    public:
        MuduoServer(int port = 9090) : _server(&_baseloop, muduo::net::InetAddress("127.0.0.1", port), "MuduoServer", muduo::net::TcpServer::kReusePort), _protocol(LVProtovolFactory::create())
        {
        }
        void start() override
        {
            _server.setConnectionCallback(std::bind(&MuduoServer::onConnection, this, std::placeholders::_1));
            _server.setMessageCallback(std::bind(&MuduoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _server.start();
            _baseloop.loop();
        }

    public:
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                std::cout << "连接建立成功" << std::endl;
                auto bcon = MuduoConnectionFactory::create(_protocol, conn);
                {
                    std::unique_lock<std::mutex>(_mutex);
                    //ILOG("---------------------------------------------------------");
                    _conn.insert(std::make_pair(conn, bcon));
                    //ILOG("---------------------------------------------------------");
                }
                if (_cb_connection)
                {
                    _cb_connection(bcon);
                }
            }
            else
            {

                std::cout << "连接断开" << std::endl;
                {
                    std::unique_lock<std::mutex>(_mutex);
                    auto it = _conn.find(conn);
                    if (it == _conn.end())
                    {
                        return;
                    }
                    BaseConnection::ptr muduo_it = it->second;
                    _conn.erase(conn);
                    if (_cb_close)
                    {
                        _cb_close(muduo_it);
                    }
                }
            }
        }
        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
        {
            BaseBuffer::ptr _buf = MudouBufferFactory::create(buf);

            while (1)
            {
                //ILOG("---------------------------------------------------------");
                if (_protocol->canProcessed(_buf) == false)
                {
                    //ILOG("---------------------------------------------------------");
                    if (_buf->readablesize() > maxDatasize)
                    {
                        ELOG("数据量过大")
                        conn->shutdown();
                    }
                    break;
                }

                BaseMessage::ptr msg;
                //ILOG("---------------------------------------------------------");
                bool ret = _protocol->OnMessage(_buf, msg);
                //ILOG("---------------------------------------------------------");
                if (ret == false)
                {
                    conn->shutdown();
                    return;
                }
                BaseConnection::ptr base;
                auto it = _conn.find(conn);

                if (it == _conn.end())
                {
                    conn->shutdown();
                    return;
                }
                base = it->second;

                if (_cb_message)
                {
                    //ILOG("---------------------------------------------------------");
                    _cb_message(base, msg);
                }
            }
        }

    private:
        const size_t maxDatasize = (1 << 16);
        BaseProtocol::ptr _protocol;
        muduo::net::EventLoop _baseloop;
        muduo::net::TcpServer _server;
        std::mutex _mutex;
        std::unordered_map<muduo::net::TcpConnectionPtr, BaseConnection::ptr> _conn;
    };
    class ServerFactory
    {
    public:
        template <typename... Args>
        static RPC::BaseServer::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
        }
    };

    class MuduoClient : public BaseClient
    {
    public:
        MuduoClient(const std::string &sip, int sport) : _baseloop(_loopthread.startLoop()), // 开始监控
                                                         _downlatch(1),                      // 初始化计数器
                                                         _client(_baseloop, muduo::net::InetAddress(sip, sport), "MuduoClient"),
                                                         _protocol(LVProtovolFactory::create())
        {
        }
        virtual void connect() override
        {
            _client.setConnectionCallback(std::bind(&MuduoClient::onConnection, this, std::placeholders::_1));
            _client.setMessageCallback(std::bind(&MuduoClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _client.connect();
            ILOG("lianjiejianli");
            _downlatch.wait();
        }
        virtual void shutdown()
        {
            ILOG("gunabilianjie");
            _client.disconnect();
        }
        virtual bool send(const BaseMessage::ptr &val) override
        {
            if (connected() == false)
            {
                ELOG("连接已关闭")
                return false;
            }
            _conn->send(val);
            return true;
        }
        virtual BaseConnection::ptr connection() override
        {
            return _conn;
        }
        virtual bool connected() override
        {
            return (_conn && _conn->connected());
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                std::cout << "连接建立成功" << std::endl;
                _downlatch.countDown();
                _conn = MuduoConnectionFactory::create(_protocol, conn);
            }
            else
            {
                std::cout << "连接断开" << std::endl;
                _conn.reset();
            }
        }
        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
        {
            BaseBuffer::ptr _buf = MudouBufferFactory::create(buf);

            while (1)
            {
                if (_protocol->canProcessed(_buf) == false)
                {
                    if (_buf->readablesize() > maxDatasize)
                    {
                        ELOG("数据量过大")
                        conn->shutdown();
                    }
                    break;
                }
                BaseMessage::ptr msg;
                bool ret = _protocol->OnMessage(_buf, msg);
                if (ret == false)
                {
                    conn->shutdown();
                    return;
                }
                if (_cb_message)
                {
                    _cb_message(_conn, msg);
                }
            }
        }

    private:
        const size_t maxDatasize = (1 << 16);
        BaseConnection::ptr _conn;
        muduo::net::EventLoopThread _loopthread;
        muduo::CountDownLatch _downlatch;
        muduo::net::EventLoop *_baseloop;
        muduo::net::TcpClient _client;
        BaseProtocol::ptr _protocol;
    };

    class ClientFactory
    {
    public:
        template <typename... Args>
        static RPC::BaseClient::ptr create(Args &&...args)
        {
            return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
        }
    };

}