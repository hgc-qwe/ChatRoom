#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <string>
using namespace std;
using json = nlohmann::json;

bool SendJOSN(const string& s, int client_fd) {
    uint32_t data_len = htonl(s.length());
    if (send(client_fd, &data_len, sizeof(data_len), 0) == -1) {    
        cerr << "send failed" << endl;
        return false;
    }
    if (send(client_fd, s.c_str(), s.length(), 0) == -1) {    
        cerr << "send failed" << endl;
        return false;
    }
    return true;
}

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        cerr << "socket failed" << endl;
        return 1;
    }

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1" , &client_addr.sin_addr.s_addr);
    socklen_t client_len = sizeof(client_addr);

    if (connect(client_fd, (struct sockaddr*)&client_addr, client_len) == -1) {
        cerr << "connect failed" << endl;
        return 1;
    }

    json j;
    j["type"] = "chat";
    j["msg"] = "hello";
    string data = j.dump();
    int count  = 5;
    while (count--) {
        SendJOSN(data, client_fd);
    }

    return 0;
}