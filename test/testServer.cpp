#include <iostream>
#include "TcpServer.h"


int main()
{
    TcpServer server(8888);


    if(!server.init())
    {
        std::cerr << "server init failed" << std::endl;
        return -1;
    }


    std::cout << "server start, port: 8888" << std::endl;


    server.start();


    return 0;
}