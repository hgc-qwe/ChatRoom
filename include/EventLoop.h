#pragma once
#include <unordered_map>
#include <memory>
#include "Epoll.h"

class Channel;

class EventLoop {
private:
    Epoll epoll;

    std::unordered_map<int, std::shared_ptr<Channel>> channels;
public:
    EventLoop();

    void loop();

    Epoll* getEpoll();

    void addChannel(std::shared_ptr<Channel> channel);

    void removeChannel(int fd);

    void updateChannel(std::shared_ptr<Channel> channel);
};