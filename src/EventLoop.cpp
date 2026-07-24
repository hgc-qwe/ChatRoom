#include <iostream>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <sys/eventfd.h>
#include "EventLoop.h"
#include "Channel.h"

EventLoop::EventLoop() {
    threadId = std::this_thread::get_id();
    wakeupFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    wakeupChannel = std::make_shared<Channel>(this, wakeupFd);
    wakeupChannel->setEvents(EPOLLIN);
    wakeupChannel->setReadCallback([this]() {
        handleRead();
    });
    addChannel(wakeupChannel);
}

EventLoop::~EventLoop() {
    close(wakeupFd);
}

void EventLoop::loop() {
    while (true) {
        auto events = epoll.poll();

        for (auto& ev : events) {
            int fd = ev.data.fd;
            
            auto it = channels.find(fd);
            if (it == channels.end()) continue;
            auto channel = it->second;
            
            channel->handleEvent(ev.events);
        }
        doPendingFunctors();
    }
}

Epoll* EventLoop::getEpoll() {
    return &epoll;
}

void EventLoop::addChannel(std::shared_ptr<Channel> channel) {
    int fd = channel->getFd();
    channels[fd] = channel;
    epoll.addFd(fd, channel->getEvents());
}

void EventLoop::removeChannel(int fd) {
    epoll.delFd(fd);
    channels.erase(fd);
}

void EventLoop::updateChannel(std::shared_ptr<Channel> channel) {
    epoll.modifyFd(channel->getFd(), channel->getEvents());
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) cb();
    else queueInLoop(cb);
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        pendingFunctors.push_back(cb);
    }
    wakeup();
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = write(wakeupFd, &one, sizeof(one));
    if (n != sizeof(one)) perror("wakeup");
}

void EventLoop::handleRead() {
    uint64_t one;
    ssize_t n = read(wakeupFd, &one, sizeof(one));
    if (n != sizeof(one)) perror("handleRead");
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    {
        std::lock_guard<std::mutex> lock(mutex);
        functors.swap(pendingFunctors);
    }
    for (auto& functor : functors) {
        functor();
    }
}

bool EventLoop::isInLoopThread() {
    return threadId == std::this_thread::get_id();
}