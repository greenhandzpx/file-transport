#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/TcpClient.h"
#include "muduo/base/Mutex.h"

#include "codec.cc"

#include <functional>
#include <string>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace std::placeholders;

const int kBufSize = 32; // 上传文件时每次读取的文件内容的大小

void onHighWaterMark(const muduo::net::TcpConnectionPtr& conn, size_t len)
{
    LOG_INFO << "HighWaterMark " << len;
}

class FileClient {
    public:
        FileClient(muduo::net::EventLoop* loop, 
                   const muduo::net::InetAddress& serverAddr,
                   std::string name)
            : loop_(loop),
              client_(loop, serverAddr, "FileClient"),
              //codec_(std::bind(&FileClient::onStringMessage, this, _1, _2, _3)),
              name_(name),
              file_flag(false)
        {
            client_.setConnectionCallback(
                std::bind(&FileClient::onConnection, this, _1)
            );
            client_.setMessageCallback(
                std::bind(&FileClient::onMessage, this, _1, _2, _3)
            );
            client_.enableRetry();

            // client_.setWriteCompleteCallback(
            //     std::bind(&FileClient::onWriteComplete, this, _1)
            // );
        }

        void connect()
        {
            client_.connect();
        }

        // 目前为空，客户端的连接由操作系统在进程终止时关闭
        void disconnect()
        {
            //client_.disconnect();
        } 

        // 客户可以上传文件到服务端
        void uploadFile(const char* g_file)
        {
            //printf("ok\n");
           //connection_->setHighWaterMarkCallback(onHighWaterMark, kBufSize+1);
            //printf("ok\n");
            file_flag = true;
            muduo::MutexLockGuard lock(mutex_);
            //printf("%s\n", g_file);
            if (connection_) {
                //printf("%s\n", g_file);
                FILE* fp = fopen(g_file, "rb");
                
                if (fp) {
                    std::string content;
                    const int kBufSize = 1024*1024;
                    char iobuf[kBufSize];
                    ::setbuffer(fp, iobuf, sizeof iobuf);

                    char buf[kBufSize];
                    size_t nread = 0;
                    while ( (nread = ::fread(buf, 1, sizeof buf, fp)) > 0)
                    {
                        content.append(buf, nread);
                    }
                    fclose(fp);

                    connection_->send(content);
                    //codec_.send(connection_.get(), buf);
                } else {
                    connection_->shutdown();
                    LOG_INFO << "FileServer - no such file";
                }                
            } 
        }

  

    private:
        // onConnection由Eventloop线程调用，需要加锁保护shared_ptr
        void onConnection(const muduo::net::TcpConnectionPtr& conn)
        {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                     << conn->peerAddress().toIpPort() << " is "
                     << (conn->connected() ? "UP" : "DOWN");
            
            muduo::MutexLockGuard lock(mutex_);
            if (conn->connected()) {
                connection_ = conn;
            } else {
                connection_.reset();
            }
        }

        // 把收到的消息打印到屏幕上，不用加锁，因为printf是线程安全的
        void onMessage(const muduo::net::TcpConnectionPtr&,
                             muduo::net::Buffer* buf,
                             muduo::Timestamp)
        {
            //printf(">>> %s\n", message.c_str());
            printf("%s\n", buf->peek());
        }    
    
        muduo::net::EventLoop* loop_;
        muduo::net::TcpClient client_;
        //LengthHeaderCodec codec_;
        muduo::MutexLock mutex_;
        muduo::net::TcpConnectionPtr connection_;    

        std::string name_; // 客户的名字   
        bool file_flag; // 是否正在上传文件的标志  
};

int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid();
    if (argc > 0) {
        std::string name = "client";
        std::string serverip;
        std::cout << "Please input the ip of the server:(like x.x.x.x)";
        std::cin >> serverip;

        muduo::net::EventLoopThread loopThread;
        // uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        // muduo::net::InetAddress serverAddr(argv[1], port);
        muduo::net::InetAddress serverAddr(serverip.c_str(), 12345);

         // startloop会创建一个线程
        FileClient client(loopThread.startLoop(), serverAddr, name);
        client.connect();

        //sleep(1);        
        //client.uploadFile(argv[3]);
        //printf("ok\n");

        std::string line;   
        std::string filename;
        std::cout << "输入 sb 即可传输文件" << std::endl;
        // 从标准输入读取用户输入
        while (std::getline(std::cin, line)) {
            if (line == "sb") {
                std::cout << "输入文件名：";
                // scanf("%s", filename);
                // fflush(stdin);
                std::getline(std::cin, filename);
                client.uploadFile(filename.data());
                //std::cout << "输入 sb 即可传输文件" << std::endl;             
            } 
        }
        client.disconnect();
    } else {
        printf("Usage: %s host_ip port\n", argv[0]);
    }
    
    return 0;
}