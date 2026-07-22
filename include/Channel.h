#pragma once
#include <functional>
#include <sys/epoll.h>

class EventLoop;

class Channel {
private:
    EventLoop* loop;
    int fd;
    uint32_t events;
    uint32_t revents;

    std::function<void()> readCallback;
    std::function<void()> writeCallback;
    std::function<void()> closeCallback;
public:
    Channel(EventLoop* loop, int fd);
    
    int getFd() const;

    uint32_t getEvents() const;

    void setEvents(uint32_t ev);

    void setReadCallback(std::function<void()> cb);

    void setWriteCallback(std::function<void()> cb);

    void setCloseCallback(std::function<void()> cb);

    void handleRead();
    void handleWrite();

    void setRevents(uint32_t ev);
};