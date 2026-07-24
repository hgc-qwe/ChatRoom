#include <iostream>
#include "TcpServer.h"


int main()
{
    TcpServer server(8888);


    if(!server.init())
    {
        std::cout 
            << "server init failed"
            << std::endl;

        return -1;
    }


    std::cout
        << "server start"
        << std::endl;


    server.start();


    return 0;
}