#include "3connection.h"

Connection::Connection() {
    fd = -1;
}

Connection::Connection(int fd) {
    this->fd = fd;
}