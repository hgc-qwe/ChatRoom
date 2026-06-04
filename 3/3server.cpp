#include "3server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <unordered_map>
using namespace std;
using json = nlohmann::json;

Server::Server(int port) {
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        cerr << "listen_fd socket failed" << endl;
        return;
    }

    struct sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        cerr << "bind failed" << endl;
        return;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (listen(listen_fd, 128) == -1) {
        cerr << "listen failed" << endl;
        return;
    }

    epfd = epoll_create(1);
    if (epfd == -1) {
        cerr << "epoll_create failed" << endl;
        return;
    }

    struct epoll_event ev;
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        cerr << "epoll_ctl failed" << endl;
        return;
    }
}

void Server::Start() {
    vector<epoll_event> events(64); 
    while (true) {
        int count = epoll_wait(epfd, events.data(), events.size(), -1);
        if (count == -1) {
            cerr << "epoll_wait failed" << endl;
            break;
        }

        for (int i = 0; i < count; i++) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                HandleAccept();
            } else {
                if (events[i].events & EPOLLIN) {
                    HandleRead(fd);
                } else if (events[i].events & EPOLLOUT) {

                } else if (events[i].events & EPOLLERR) {
                    HandleClose(fd);
                }
            }
        }
    }
}

void Server::HandleAccept() {
    int client_fd = accept(listen_fd, nullptr, nullptr);
    if (client_fd == -1) return;
    Connection conn;
    conn.fd = client_fd;
    clients[client_fd] = conn;

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);

    cout << "新客户端连接：" << client_fd << endl;
}

void Server::HandleRead(int fd) {
    char buf[BUFSIZ];
    int ret = recv(fd, buf, sizeof(buf) - 1, 0);
    if (ret <= 0) {
        HandleClose(fd);
        return;
    }

    buf[ret] = '\0';
    Connection& conn = clients[fd];
    conn.readBuffer.append(buf, ret);
                        
    while (true) {
        if (conn.readBuffer.size() < 4) break;
        uint32_t len;
        memcpy(&len, conn.readBuffer.data(), 4);
        len = ntohl(len);

        if (conn.readBuffer.size() < 4 + len) break;
        string body = conn.readBuffer.substr(4, len);
        conn.readBuffer.erase(0, 4 + len);
        json j = json::parse(body);
        cout << j.dump() << endl;
    }
}

void Server::HandleClose(int fd) {
    clients.erase(fd);
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    cout << "客户端 " << fd << " 已断开" << endl;
}

void Server::Broadcast(int senderFd, const string& msg) {
    for (auto& [conn_fd, conn] : clients) {
        if (conn_fd != senderFd) {
            if (send(conn_fd,msg.c_str(), msg.length(), 0) <= 0) continue;
        }
    }
}