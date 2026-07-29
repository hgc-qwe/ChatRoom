#include <iostream>
#include "TcpServer.h"
#include "Logger.h"


int main()
{
    Logger::init();


    TcpServer server(8080);


    if(!server.init())
    {
        LOG_ERROR("server init failed");
        return -1;
    }


    LOG_INFO("server start success");


    server.start();


    return 0;
}