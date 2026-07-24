#pragma once
#include <vector>
#include <memory>
#include "EventLoopThread.h"
#include "EventLoop.h"

class EventLoopThreadPool {
public:
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads);

    void start();

    EventLoop* getNextLoop();
private:
    EventLoop* baseLoop;

    int numThreads;

    int next;

    std::vector<std::unique_ptr<EventLoopThread>> threads;

    std::vector<EventLoop*> loops;
};