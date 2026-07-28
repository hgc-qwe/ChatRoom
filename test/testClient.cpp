#include <iostream>
#include <string>

#include <unistd.h>
#include <arpa/inet.h>


#include "chat.pb.h"
#include "MessageCodec.h"


int main()
{

    int sockfd = socket(
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

    serverAddr.sin_port = htons(8888);

    serverAddr.sin_addr.s_addr =
        inet_addr("127.0.0.1");



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
        << "connect server success"
        << std::endl;



    /*
        构造登录请求
    */

    chat::LoginReq req;


    req.set_id(1);

    req.set_password("123");



    std::string data;


    req.SerializeToString(
        &data
    );



    /*
        加协议头

        msgid + length + data
    */

    std::string message =
        MessageCodec::encode(
            chat::LOGIN_MSG,
            data
        );



    int n = send(
        sockfd,
        message.data(),
        message.size(),
        0
    );


    std::cout
        << "send bytes:"
        << n
        << std::endl;



    /*
        接收服务器响应
    */


    char buffer[4096]={0};

    int len = recv(
        sockfd,
        buffer,
        sizeof(buffer),
        0
    );


    if(len > 0)
    {
        std::cout 
            << "recv bytes:"
            << len
            << std::endl;


        int msgid;
        std::string data;


        Buffer bufferRecv;

        bufferRecv.append(
            buffer,
            len
        );


        if(MessageCodec::decode(
            bufferRecv,
            msgid,
            data))
        {

            std::cout
                << "msgid:"
                << msgid
                << std::endl;


            if(msgid == chat::LOGIN_MSG_ACK)
            {
                chat::LoginRes res;

                res.ParseFromString(data);


                std::cout
                    << "err:"
                    << res.err()
                    << std::endl;


                std::cout
                    << "errmsg:"
                    << res.errmsg()
                    << std::endl;


                if(res.err()==0)
                {
                    std::cout
                        << "userid:"
                        << res.user().id()
                        << std::endl;


                    std::cout
                        << "username:"
                        << res.user().name()
                        << std::endl;

                    std::cout 
                        << "friends:"
                        << res.friends_size()
                        << std::endl;


                    for(int i=0;i<res.friends_size();i++)
                    {
                        auto friendUser = res.friends(i);

                        std::cout
                            << "friend id:"
                            << friendUser.id()
                            << " name:"
                            << friendUser.name()
                            << " state:"
                            << friendUser.state()
                            << std::endl;
                    }



                    std::cout
                        << "groups:"
                        << res.groups_size()
                        << std::endl;


                    for(int i=0;i<res.groups_size();i++)
                    {
                        auto group = res.groups(i);

                        std::cout
                            << "group id:"
                            << group.id()
                            << " name:"
                            << group.name()
                            << " desc:"
                            << group.desc()
                            << std::endl;
                    }



                    std::cout
                        << "offline msg:"
                        << res.offlinemsgs_size()
                        << std::endl;


                    for(auto& msg:res.offlinemsgs())
                    {
                        std::cout
                            << msg
                            << std::endl;
                    }
                }
            }
        }
    }


        


    close(sockfd);


    return 0;
}