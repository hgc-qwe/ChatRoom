#pragma once
#include <string>

class Connection
{
public:
    int fd;
    std::string username;
    std::string readBuffer;

public:
    Connection();
    Connection(int fd);
};