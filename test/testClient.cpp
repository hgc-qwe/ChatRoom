#include <iostream>
#include <string>

#include <unistd.h>
#include <arpa/inet.h>


int main()
{

    int sockfd =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );


    if(sockfd < 0)
    {
        perror("socket");
        return -1;
    }



    sockaddr_in serverAddr{};


    serverAddr.sin_family = AF_INET;

    serverAddr.sin_port =
        htons(8888);


    inet_pton(
        AF_INET,
        "127.0.0.1",
        &serverAddr.sin_addr
    );



    if(connect(
        sockfd,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr)
    ) < 0)
    {
        perror("connect");
        return -1;
    }



    std::cout
        << "connect success"
        << std::endl;



    while(true)
    {

        std::string msg;


        std::cout
            << "input:"
            << std::endl;


        std::getline(
            std::cin,
            msg
        );



        if(msg == "quit")
            break;



        send(
            sockfd,
            msg.data(),
            msg.size(),
            0
        );



        char buf[1024];

        int n =
            recv(
                sockfd,
                buf,
                sizeof(buf),
                0
            );


        if(n > 0)
        {
            std::cout
                << "server:"
                << std::string(buf,n)
                << std::endl;
        }


        

    }


    close(sockfd);


    return 0;
}