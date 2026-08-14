#include <iostream>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include "EventLoop.h"
#include "Channel.h"
#include "TcpConnection.h"

EventLoop::EventLoop() : wakeupFd(-1), timerFd(-1) {
    std::cout
        << "[EventLoop] create thread = "
        << std::this_thread::get_id()
        << std::endl;
    threadId = std::this_thread::get_id();
    wakeupFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    wakeupChannel = std::make_shared<Channel>(this, wakeupFd);
    wakeupChannel->setEvents(EPOLLIN);
    wakeupChannel->setReadCallback([this]() {
        handleRead();
    });
    addChannel(wakeupChannel);

    timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerFd == -1) {
        perror("timerfd_create");
        return;
    }
    timerChannel = std::make_shared<Channel>(this, timerFd);
    timerChannel->setEvents(EPOLLIN);
    timerChannel->setReadCallback([this]() {
        handleTimer();
    });
    addChannel(timerChannel);
    addTimer(10);
}

EventLoop::~EventLoop() {
    if (wakeupFd != -1) close(wakeupFd);
    if (timerFd != -1) close(timerFd);
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

void EventLoop::addTimer(int interval) {
    struct itimerspec newValue{};
    newValue.it_value.tv_sec = interval;
    newValue.it_interval.tv_sec = interval;
    if (timerfd_settime(timerFd, 0, &newValue, nullptr) == -1) perror("timerfd_settime");
}

void EventLoop::handleTimer() {
    uint64_t expirations;
    ssize_t n = read(timerFd, &expirations, sizeof(expirations));
    if (n != sizeof(expirations)) {
        perror("timer read");
        return;
    }

    std::vector<std::shared_ptr<TcpConnection>> timeoutConnections;
    std::vector<std::shared_ptr<TcpConnection>> activeConnections;

    for (auto it = connections.begin(); it != connections.end(); ) {
        auto conn = it->second.lock();
        if (!conn) {
            it = connections.erase(it);
            continue;
        }
        if (conn->isTimeout()) timeoutConnections.push_back(conn);
        else activeConnections.push_back(conn);

        ++it;
    }

    for (auto& conn : timeoutConnections) {
        std::cout << "[Heartbeat] timeout fd = " << conn->getFd() << std::endl;
        conn->handleClose();
    }

    for (auto& conn : activeConnections) conn->sendPing();
}

void EventLoop::addConnection(std::shared_ptr<TcpConnection> conn) {
    connections[conn->getFd()] = conn;
}

void EventLoop::removeConnection(int fd) {
    connections.erase(fd);
}