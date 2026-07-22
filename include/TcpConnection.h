#pragma once
#include <string>
#include <functional>
#include <memory>
#include "Channel.h"

class Epoll;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(int fd, EventLoop* loop);
    ~TcpConnection();

    bool sendMessage(const std::string& msg);

    bool recvMessage();

    void sendBuffer();
    std::string& getReadBuffer();

    void close();

    int getFd() const;

    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, std::string&)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;

    void setConnectionCallback(ConnectionCallback cb);
    void setMessageCallback(MessageCallback cb);
    void setCloseCallback(CloseCallback cb);
    void connectEstablished();
private:
    int fd;
    EventLoop* loop;
    std::string readBuffer;
    std::string writeBuffer;
    std::shared_ptr<Channel> channel;

    ConnectionCallback connectionCallback;
    MessageCallback messageCallback;
    CloseCallback closeCallback;
};