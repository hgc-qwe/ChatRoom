#include <vector>
#include <sys/epoll.h>
#pragma once

class Epoll {
private:
    int epfd;
public:
    Epoll();
    ~Epoll();

    bool addFd(int fd, uint32_t events);
    bool delFd(int fd);
    std::vector<epoll_event> poll();
    bool modifyFd(int fd, uint32_t events);
};