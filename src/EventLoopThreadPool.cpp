#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, int numThreads) {
    this->baseLoop = baseLoop;
    this->numThreads = numThreads;
    next = 0;
}

void EventLoopThreadPool::start() {
    for (int i = 0; i < numThreads; i++) {
        auto thread = std::make_unique<EventLoopThread>();
        EventLoop* loop = thread->startLoop();
        loops.push_back(loop);
        threads.push_back(std::move(thread));
    }
}

EventLoop* EventLoopThreadPool::getNextLoop() {
    if (loops.empty()) return baseLoop;

    EventLoop* loop = loops[next];
    next++;
    if (next >= loops.size()) next = 0;
    return loop;
}