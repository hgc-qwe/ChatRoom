#include <sys/socket.h>
#include <unistd.h>
#include "TcpServer.h"
#include "TcpConnection.h"
#include "EventLoop.h"

TcpConnection::TcpConnection(int fd, EventLoop* loop) {
    this->fd = fd;
    this->loop = loop;

    channel = std::make_shared<Channel>(fd);
    channel->setEvents(EPOLLIN);
    channel->setReadCallback([this]() {
        recvMessage();
    });
    channel->setWriteCallback([this]() {
        sendBuffer();
    });
}

TcpConnection::~TcpConnection() {
    if (fd != -1) ::close(fd);
}

bool TcpConnection::sendMessage(const std::string& msg) {
    writeBuffer += msg;
    
    channel->setEvents(EPOLLIN | EPOLLOUT);
    loop->updateChannel(channel);
    return true;
}

bool TcpConnection::recvMessage() {
    while (true) {
        char buf[BUFSIZ];
        int count = recv(fd, buf, sizeof(buf), 0);

        if (count == 0) {
            if (closeCallback) closeCallback(shared_from_this());
            return false;
        }
        if (count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
            perror("recv");
            return false;
        } else {
            readBuffer.append(buf, count);
        }
    }

    if (messageCallback) {
        messageCallback(shared_from_this(), readBuffer);
    }
    return true;
}

void TcpConnection::close() {
    ::close(fd);
}

int TcpConnection::getFd() const {
    return fd;
}

void TcpConnection::sendBuffer() {
    if (writeBuffer.empty()) return;

    int n = send(fd, writeBuffer.data(), writeBuffer.size(), 0);
    if (n > 0) {
        writeBuffer.erase(0, n);
    }
    if (n <= 0) {
        if (errno == EAGAIN) return;
        close();
    }
    if (writeBuffer.empty()) {
        channel->setEvents(EPOLLIN);
        loop->updateChannel(channel);
    }
}

std::string& TcpConnection::getReadBuffer() {
    return readBuffer;
}

void TcpConnection::setConnectionCallback(ConnectionCallback cb) {
    connectionCallback = cb;
}

void TcpConnection::setMessageCallback(MessageCallback cb) {
    messageCallback = cb;
}

void TcpConnection::setCloseCallback(CloseCallback cb) {
    closeCallback = cb;
}

void TcpConnection::connectEstablished() {
    loop->addChannel(channel);

    if (connectionCallback) {
        connectionCallback(shared_from_this());
    }
}