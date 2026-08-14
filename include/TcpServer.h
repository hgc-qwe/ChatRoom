#pragma once
#include <string>
#include <unordered_map>
#include <netinet/in.h>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <openssl/ssl.h>
#include "TcpConnection.h"
#include "Epoll.h"
#include "EventLoop.h"
#include "Dispatcher.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

class TcpServer {
private:
    SSL_CTX* sslCtx{nullptr};
    int listenfd{-1};
    int port;
    std::string ip;
    EventLoop loop;
    std::mutex connMutex;
    EventLoopThreadPool threadPool;
    struct sockaddr_in listen_addr;
    Dispatcher dispatcher;
    std::unordered_map<int, int> clients;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections;

    bool createListenFd();
    bool bind();
    bool listen();
    void acceptConnection();
    void handleRead(int fd);
    void handleWrite(int fd);
    void closeConnection(int fd);
    bool initSSL();

    void removeConnection(std::shared_ptr<TcpConnection> conn);
public:
    TcpServer(const std::string& ip, int port);
    ~TcpServer();
    bool init();
    void start();
};