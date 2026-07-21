#include <vector>
#include <sys/epoll.h>

class Epoll {
private:
    int epfd;
public:
    Epoll();
    ~Epoll();

    bool addFd(int fd);
    bool delFd(int fd);
    std::vector<epoll_event> poll();
};