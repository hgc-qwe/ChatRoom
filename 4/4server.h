#pragma once
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "3connection.h"
using json = nlohmann::json;

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
    void HandleMessage(int fd, const json& j);
    void HandleLogin(int fd, const json& j);
    void HandleChat(int fd, const json& j);
    void HandleRegister(int fd, const json& j);
    void SendErr();

    void Broadcast(int senderFd,const std::string& msg);

private:
    int listen_fd;
    int epfd;

    std::unordered_map<int, Connection> clients;
};