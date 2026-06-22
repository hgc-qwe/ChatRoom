#include <iostream>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <vector>
#include <thread>
#include <unistd.h>
using namespace std;

void worker(int epfd) {
    vector<epoll_event> events(64);
    char buf[BUFSIZ];

    while (true) {
        int count = epoll_wait(epfd, events.data(), 64, -1);
        if (count < 0) {
            cerr << "epoll_wait failed" << endl;
            break;
        }

        for (int i = 0; i < count; i++) {
            int fd = events[i].data.fd;

            if (events[i].events & EPOLLIN) {

            }
            if (events[i].events & EPOLLERR) {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
            }
        }
    }
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        cerr << "listen_fd socket failed" << endl;
        return 1;
    }

    struct sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(2100);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        cerr << "listen_fd bind failed" << endl;
        return 1;
    }

    if (listen(listen_fd, 128) == -1) {
        cerr << "listen_fd listen failed" << endl;
        return 1;
    }

    int ep_fd = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        cerr << "listen_fd epoll_ctl failed" << endl;
        return 1;
    }

    int worker_num = 3;
    vector<int> epoll_fds(worker_num);
    vector<thread> threads;

    for (int i = 0; i < worker_num; i++) {
        epoll_fds[i] = epoll_create(1);
        threads.emplace_back(worker, epoll_fds[i]);
    }

    vector<epoll_event> events(64);
    int idx = 0;

    while (true) {
        int count = epoll_wait(ep_fd, events.data(), 64, -1);
        if (count < 0) {
            cerr << "epoll_wait failed" << endl;
            break;
        }

        for (int i = 0; i < count; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                int client_fd = accept(fd, nullptr, nullptr);
                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fds[i], EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    cerr << "client_fd epoll_ctl failed" << endl;
                    continue;
                }
                idx = (idx + 1) % worker_num;
            }
        }
    }
    close(listen_fd);
    return 0;
}