#include "muduo/net/TcpConnection.h"
#include "muduo/net/Buffer.h"
#include "muduo/base/Logging.h"

#include <iostream>
#include <stdio.h>

#include <functional>

//const int32_t kHeaderLen = 4;

// 这是一个用来打包和分包的类（编解码）， 是一个工具类
class LengthHeaderCodec {
    public:
        typedef std::function<void (const muduo::net::TcpConnectionPtr&,
                                    const muduo::string& message,
                                    muduo::Timestamp)> StringMessageCallback;
        explicit LengthHeaderCodec(const StringMessageCallback& cb)
            : messageCallback_(cb)
        {}

    // 发送时打包
    void send(muduo::net::TcpConnection* conn,
                const muduo::StringPiece& message)
    {
        muduo::net::Buffer buf;
        //printf("%s\n", message.data());
        buf.append(message.data(), message.size());
        int32_t len = static_cast<int32_t>(message.size());
       // printf("%d\n", len);
        int32_t be32 = muduo::net::sockets::hostToNetwork32(len); // 转成网络字节序
       // printf("%d\n", be32);
        buf.prepend(&be32, sizeof(be32));
        conn->send(&buf);
    }

    // 接收时分包
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                    muduo::net::Buffer* buf,
                    muduo::Timestamp receiveTime)
    {
        while (buf->readableBytes() >= kHeaderLen) {
            const void* data = buf->peek();
            int32_t be32 = *static_cast<const int32_t*>(data);
            const int32_t len = muduo::net::sockets::networkToHost32(be32);
            if (len > 65536 || len < 0) {
                LOG_ERROR << "Invalid length " << len;
                conn->shutdown();
                break; 
            } else if (buf->readableBytes() >= len + kHeaderLen) {
                buf->retrieve(kHeaderLen); // 移动指针位置到消息正文
                muduo::string message(buf->peek(), len);
                //std::cout << message << std::endl;
                messageCallback_(conn, message, receiveTime);
                buf->retrieve(len);
            }
        }
    }
    private:
        StringMessageCallback messageCallback_;
        const static size_t kHeaderLen = sizeof(int32_t);
};






