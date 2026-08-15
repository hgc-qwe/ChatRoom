#include <iostream>
#include <string>
#include "TcpServer.h"
#include "Logger.h"


int main(int argc, char* argv[]) {
    Logger::init();

   std::string ip = "127.0.0.1";
   int port = 8000;

   if (argc >= 2) ip = argv[1];
   if (argc >= 3) port = std::stoi(argv[2]);
   if (argc >= 4) {
        std::cerr << "invailed ip : port" << std::endl;
        return -1;
    }

    if (port < 1 || port > 65535) {
        std::cerr << "invalid port: " << port << std::endl;
        return -1;
    }
   
   std::cout << "server address: " << ip << ":" << port << std::endl;

   TcpServer server(ip, port);
    if(!server.init()) {
        LOG_ERROR("server init failed");
        return -1;
    }
    LOG_INFO("server start success");
    server.start();

    return 0;
}