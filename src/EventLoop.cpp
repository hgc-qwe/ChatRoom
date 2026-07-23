#include <iostream>
#include "EventLoop.h"
#include "Channel.h"

EventLoop::EventLoop() {

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