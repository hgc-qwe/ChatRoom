#include "Util.h"
#include <fcntl.h>
#include <iostream>

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