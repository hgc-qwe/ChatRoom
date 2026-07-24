#include "EventLoopThread.h"

EventLoopThread::EventLoopThread() {
    loop = nullptr;
}

EventLoop* EventLoopThread::startLoop() {
    thread = std::thread(&EventLoopThread::threadFunc, this);
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]() {
            return loop != nullptr;
        });
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop eventLoop;
    {
        std::lock_guard<std::mutex> lock(mutex);
        this->loop = &eventLoop;
        cv.notify_one();
    }
    eventLoop.loop();
}