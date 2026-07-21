#include <iostream>
#include <unistd.h>
#include "Epoll.h"

Epoll::Epoll() {
    epfd = epoll_create1(0);
    if (epfd == -1) perror("epoll_create1");
}

bool Epoll::addFd(int fd) {
    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl");
        return false;
    }
    
    return true;
}

bool Epoll::delFd(int fd) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        perror("epoll_ctl del");
        return false;
    }
    close(fd);
    return true;
}

std::vector<epoll_event> Epoll::poll() {
    std::vector<epoll_event> events(64);

    int count = epoll_wait(epfd, events.data(), 64, -1);
    if (count < 0) {
        perror("epoll_wait");
        return {};
    }
    
    events.resize(count);

    return events;
}

Epoll::~Epoll() {
    if (epfd != -1) close(epfd);
}