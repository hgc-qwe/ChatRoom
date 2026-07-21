#pragma once
#include <string>
#include <unordered_map>
#include <netinet/in.h>
#include "Epoll.h"
#include "Dispatcher.h"

class TcpServer {
private:
    int listenfd;
    int port;
    Epoll epoll;
    struct sockaddr_in listen_addr;
    Dispatcher dispatcher;
    std::unordered_map<int, int> clients;

    bool createListenFd();
    bool bind();
    bool listen();
    void acceptConnection();
    void handleRead(int fd);
    void closeConnection(int fd);
public:
    TcpServer(int port);
    ~TcpServer();
    bool init();
    void start();
};