#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"

class EventLoopThread {
public:
    EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop* loop;
    std::thread thread;
    std::mutex mutex;
    std::condition_variable cv;
};