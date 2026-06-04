#pragma once
#include <unordered_map>
#include "3connection.h"

class Server
{
public:
    Server(int port);
    ~Server();
    void Start();

private:
    void HandleAccept();
    void HandleRead(int fd);
    void HandleClose(int fd);

    void Broadcast(
        int senderFd,
        const std::string& msg
    );

private:
    int listen_fd;
    int epfd;

    std::unordered_map<int, Connection> clients;
};