#pragma once
#include <string>
#include <functional>
#include <memory>
#include "Channel.h"
#include "Buffer.h"

class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(int fd, EventLoop* loop);
    ~TcpConnection();

    bool sendMessage(const std::string& msg);

    bool recvMessage();

    void sendBuffer();
    Buffer& getReadBuffer();

    void close();

    int getFd() const;

    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, Buffer&)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;

    void setConnectionCallback(ConnectionCallback cb);
    void setMessageCallback(MessageCallback cb);
    void setCloseCallback(CloseCallback cb);
    void connectEstablished();
    void removeChannel();
private:
    int fd;
    EventLoop* loop;
    Buffer readBuffer;
    Buffer writeBuffer;
    std::shared_ptr<Channel> channel;

    ConnectionCallback connectionCallback;
    MessageCallback messageCallback;
    CloseCallback closeCallback;
};