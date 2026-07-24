#include <sys/socket.h>
#include <unistd.h>
#include "TcpServer.h"
#include "TcpConnection.h"
#include "EventLoop.h"

TcpConnection::TcpConnection(int fd, EventLoop* loop) {
    this->fd = fd;
    this->loop = loop;

    channel = std::make_shared<Channel>(loop, fd);
    channel->setEvents(EPOLLIN);
    channel->setReadCallback([this]() {
        recvMessage();
    });
    channel->setWriteCallback([this]() {
        sendBuffer();
    });
    loop->addChannel(channel);
}

TcpConnection::~TcpConnection() {
    close();
}

bool TcpConnection::sendMessage(const std::string& msg) {
    writeBuffer.append(msg);
    channel->enableWriting();
    return true;
}

bool TcpConnection::recvMessage() {
    char buf[BUFSIZ];
    while (true) {
        int count = recv(fd, buf, sizeof(buf), 0);

        if (count > 0) {
            readBuffer.append(buf, count);
        } else if (count == 0) {
            if (closeCallback) closeCallback(shared_from_this());
            return false;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("recv");
            if (closeCallback) closeCallback(shared_from_this());
            return false;
        }
    }

    if (messageCallback) {
        messageCallback(shared_from_this(), readBuffer);
    }
    return true;
}

void TcpConnection::close() {
    if (fd != -1) {
        loop->removeChannel(fd);
        ::close(fd);
        fd = -1;
    }
}

int TcpConnection::getFd() const {
    return fd;
}

void TcpConnection::sendBuffer() {
    while (writeBuffer.readableBytes() > 0) {
        int n = send(fd, writeBuffer.beginRead(), writeBuffer.readableBytes(), 0);
        if (n > 0) {
            writeBuffer.retrieve(n);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            perror("send");
            close();
            return;
        }
    }
    channel->disableWriting();
}

Buffer& TcpConnection::getReadBuffer() {
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
    if (connectionCallback) {
        connectionCallback(shared_from_this());
    }
}

void TcpConnection::removeChannel() {
    loop->removeChannel(fd);
}