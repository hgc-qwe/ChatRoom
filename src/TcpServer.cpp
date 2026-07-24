#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include "TcpServer.h"
#include "MessageCodec.h"
#include "Channel.h"
#include "Util.h"
#include <fcntl.h>

TcpServer::TcpServer(int port) : threadPool(&loop, 3) {
    this->port = port;
    listenfd = -1;
}

TcpServer::~TcpServer() {
    if (listenfd != -1) close(listenfd);
}

bool TcpServer::init() {
    if (!createListenFd()) return false;

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (!bind()) {
        return false;
    }

    if (!listen()) {
        return false;
    }

    auto channel = std::make_shared<Channel>(&loop, listenfd);
    channel->setEvents(EPOLLIN);

    channel->setReadCallback([this]() {
        acceptConnection();
    });

    loop.addChannel(channel);

    return true;
}

void TcpServer::start() {
    threadPool.start();
    loop.loop();
}

bool TcpServer::createListenFd() {
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        return false;
    }
    if (setNonBlock(listenfd) == -1) return false;
    return true;
}

bool TcpServer::bind() {
    if (::bind(listenfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        std::cerr << "listen_fd bind failed" << std::endl;
        return false;
    }
    return true;
}

bool TcpServer::listen() {
    if (::listen(listenfd, 128) == -1) {
        std::cerr << "listen failed" << std::endl;
        return false;
    }
    return true;
}

void TcpServer::acceptConnection() {
    while(true) {
        int clientfd = ::accept4(listenfd, nullptr, nullptr, SOCK_NONBLOCK);
        if (clientfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            return;
        }

        EventLoop* ioLoop = threadPool.getNextLoop();
        ioLoop->runInLoop([this, clientfd, ioLoop]() {
            auto conn = std::make_shared<TcpConnection>(clientfd, ioLoop);
            {
                std::lock_guard<std::mutex> lock(connMutex);
                connections[clientfd] = conn;
            }

            conn->setMessageCallback([this](auto conn, Buffer& buffer) {
                int msgid;
                std::string data;

                while (MessageCodec::decode(buffer, msgid, data)) {
                    auto response = dispatcher.dispatch(msgid, data);
                    conn->sendMessage(response);
                } 
            });

            conn->setCloseCallback([this](std::shared_ptr<TcpConnection> conn) {
                removeConnection(conn);
            });
            
            conn->connectEstablished();
        });
    }
}

void TcpServer::closeConnection(int fd) {
    auto conn = connections[fd];
    connections.erase(fd);
    loop.getEpoll()->delFd(fd);
}

void TcpServer::removeConnection(std::shared_ptr<TcpConnection> conn) {
    int fd = conn->getFd();
    {
        std::lock_guard<std::mutex> lock(connMutex);
        connections.erase(fd);
    }
    conn->close();
}