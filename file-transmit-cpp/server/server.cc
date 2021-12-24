#include "codec.cc"

#include "muduo/net/TcpServer.h"
#include "muduo/net/EventLoop.h"
#include <set>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <unordered_set>
#include <memory>
#include <sys/time.h>
#include <stdlib.h>

#include <boost/circular_buffer.hpp>

using namespace std::placeholders;

class FileServer {
    public:
        FileServer(muduo::net::EventLoop* loop,
                   const muduo::net::InetAddress& listenAddr)
            : loop_(loop),
              server_(loop, listenAddr, "FileServer")
              //codec_(std::bind(&FileServer::onMessage, this, _1, _2, _3))
              
        {
            server_.setConnectionCallback(
                std::bind(&FileServer::onConnection, this, _1)
            );
            server_.setMessageCallback(
                std::bind(&FileServer::onMessage, this, _1, _2, _3)
            );
        }
        
        void start()
        {
            server_.start();
        }

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn)
        {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                     << conn->peerAddress().toIpPort() << " is "
                     << (conn->connected() ? "UP" : "DOWN");            
        }

        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                  muduo::net::Buffer* buf,
                  muduo::Timestamp)
        {
            std::string name = "newfile" + std::to_string(rand() % 10000);
            FILE* fp = fopen(name.c_str(), "ab");
            //printf("%s\n", buf->peek());
            fwrite(buf->peek(), sizeof(char), buf->readableBytes() * sizeof(char), fp);
            fclose(fp);
            std::string msg("发送成功\n输入 sb 即可传输文件");
            conn->send(msg);
            //codec_.send(&conn, msg);
        }

        muduo::net::TcpServer server_;
        muduo::net::EventLoop* loop_;
        //LengthHeaderCodec codec_;
};

int main(int argc, char* argv[])
{
    srand(time(NULL));
    
    LOG_INFO << "pid = " << getpid();
    if (argc > 0) {
        muduo::net::EventLoop loop;
        //uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        uint16_t port = static_cast<uint16_t>(12345);
        muduo::net::InetAddress serverAddr(port);
        FileServer server(&loop, serverAddr);
        server.start();
        loop.loop();
    } else {
        printf("Usage: %s port\n", argv[0]);
    }
    return 0;
}