#pragma once
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <thread>
#include "Epoll.h"

class Channel;

class EventLoop {
public:
    EventLoop();

    ~EventLoop();

    void loop();

    Epoll* getEpoll();

    void addChannel(std::shared_ptr<Channel> channel);

    void removeChannel(int fd);

    void updateChannel(std::shared_ptr<Channel> channel);

    using Functor = std::function<void()>;

    void runInLoop(Functor cb);

    void queueInLoop(Functor cb);
private:
    Epoll epoll;

    int wakeupFd;

    std::shared_ptr<Channel> wakeupChannel;

    std::mutex mutex;

    std::vector<Functor> pendingFunctors;

    std::thread::id threadId;

    std::unordered_map<int, std::shared_ptr<Channel>> channels;

    void wakeup();

    void handleRead();

    void doPendingFunctors();

    bool isInLoopThread();
};