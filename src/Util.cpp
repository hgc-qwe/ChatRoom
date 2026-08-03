#include "Util.h"
#include <fcntl.h>
#include <iostream>
#include <string>

int setNonBlock(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        perror("fcntl get");
        return -1;
    }
    flag |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flag) == -1) {
        perror("fcntl set");
        return -1;
    }

    return 0;
}

std::string getCurrentTime()
{
    char buf[32];
    time_t now = time(nullptr);

    strftime(buf, sizeof(buf), "%F %T", localtime(&now));

    return buf;
}