#include "Channel.h"
#include "EventLoop.h"

Channel::Channel(EventLoop* loop, int fd) {
    this->loop = loop;
    this->fd = fd;
    events = 0;
}
    
int Channel::getFd() const {
    return fd;
}

uint32_t Channel::getEvents() const {
    return events;
}

void Channel::setEvents(uint32_t ev) {
    events = ev;
}

void Channel::setReadCallback(std::function<void()> cb) {
    readCallback = cb;
}

void Channel::setWriteCallback(std::function<void()> cb) {
    writeCallback = cb;
}

void Channel::setCloseCallback(std::function<void()> cb) {
    closeCallback = cb;
}

void Channel::setRevents(uint32_t ev) {
    revents = ev;
}

void Channel::handleRead() {
    if (readCallback) {
        readCallback();
    }
}

void Channel::handleWrite() {
    if (writeCallback) {
        writeCallback();
    }
}

void Channel::handleEvent(uint32_t events) {
    if (events & (EPOLLHUP | EPOLLERR)) {
        if (closeCallback) closeCallback();
        return;
    }
    if (events & EPOLLIN) {
        handleRead();
    }
    if (events & EPOLLOUT) {
        handleWrite();
    }
}

void Channel::enableReading() {
    events |= EPOLLIN;
    loop->updateChannel(shared_from_this());
}

void Channel::enableWriting() {
    events |= EPOLLIN;
    loop->updateChannel(shared_from_this());
}

void Channel::disableWriting() {
    events &= ~EPOLLOUT;
    loop->updateChannel(shared_from_this());
}