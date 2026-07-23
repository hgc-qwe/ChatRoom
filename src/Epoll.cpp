#include <iostream>
#include <unistd.h>
#include "Epoll.h"

Epoll::Epoll() {
    epfd = epoll_create1(0);
    if (epfd == -1) perror("epoll_create1");
}

bool Epoll::modifyFd(int fd, uint32_t events) {
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        perror("epoll_ctl mod");
        return false;
    }
    
    return true;
}

bool Epoll::addFd(int fd, uint32_t events) {
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    
    return epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoll::delFd(int fd) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        perror("epoll_ctl del");
        return false;
    }
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