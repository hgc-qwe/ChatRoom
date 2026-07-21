#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include "TcpServer.h"
#include "MessageCodec.h"

TcpServer::TcpServer(int port) {
    this->port = port;
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

    epoll.addFd(listenfd);

    return true;
}

void TcpServer::start() {
    while (true) {
        auto events = epoll.poll();

        for (auto& ev : events) {
            int fd = ev.data.fd;
            
            if (fd == listenfd) {
                acceptConnection();
            } else {
                handleRead(fd);
            }
        }
    }
}

bool TcpServer::createListenFd() {
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        std::cerr << "listen_fd socket failed" << std::endl;
        return false;
    }
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
    int clientfd = accept(listenfd, nullptr, nullptr);
    if (clientfd == -1) {
        perror("accept");
        return;
    }

    if (epoll.addFd(clientfd)) {
        std::cout << "add client fd success" << std::endl;
    } else {
        std::cout << "add client fd failed" << std::endl;
    }
}

void TcpServer::handleRead(int fd) {
    char buf[BUFSIZ];
    ssize_t count = recv(fd, buf, sizeof(buf), 0);
    if (count > 0) {
        std::string buffer(buf, count);
        int msgid;
        std::string data;
        if (MessageCodec::decode(buffer, msgid, data)) {
            std::string response = dispatcher.dispatch(msgid, data);
            send(fd, response.data(), response.size(), 0);
        } else {
            std::cerr << "decode failed" << std::endl;
        }
    } else if (count == 0) {
        closeConnection(fd);
    } else {
        perror("recv");
        closeConnection(fd);
    }
}

void TcpServer::closeConnection(int fd) {
    epoll.delFd(fd);
}