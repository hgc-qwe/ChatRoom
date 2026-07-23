#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include "Buffer.h"
#include "MessageCodec.h"
#include "chat.pb.h"


int main()
{
    int sockfd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if(sockfd == -1)
    {
        perror("socket");
        return -1;
    }


    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);
    server.sin_addr.s_addr =
        inet_addr("127.0.0.1");


    if(connect(
        sockfd,
        (sockaddr*)&server,
        sizeof(server)
    ) == -1)
    {
        perror("connect");
        return -1;
    }


    std::cout
        <<"connect server success"
        <<std::endl;



    // 构造protobuf请求
    chat::LoginReq req;

    req.set_msgid(chat::LOGIN_MSG);
    req.set_id(1);
    req.set_password("123");


    // protobuf序列化
    std::string data;

    if(!req.SerializeToString(&data))
    {
        std::cout
            <<"serialize failed"
            <<std::endl;

        return -1;
    }



    // MessageCodec编码
    std::string msg =
        MessageCodec::encode(
            chat::LOGIN_MSG,
            data
        );



    // 发送
    int n = send(
        sockfd,
        msg.data(),
        msg.size(),
        0
    );


    std::cout
        <<"send bytes:"
        <<n
        <<std::endl;



    // 接收服务器返回
    Buffer recvBuffer;
    char buf[1024];

    while (true) {

        int len = recv(
            sockfd,
            buf,
            sizeof(buf),
            0
        );


        if(len > 0)
        {
            recvBuffer.append(
                buf,
                len
            );


            int msgid;
            std::string data;


            while(
                MessageCodec::decode(
                    recvBuffer,
                    msgid,
                    data
                )
            )
            {
                std::cout
                    <<"recv msgid:"
                    <<msgid
                    <<std::endl;


                if(msgid == chat::LOGIN_MSG_ACK)
                {
                    chat::LoginRes res;

                    if(res.ParseFromString(data))
                    {
                        std::cout
                            <<"err:"
                            <<res.err()
                            <<std::endl;


                        std::cout
                            <<"msg:"
                            <<res.errmsg()
                            <<std::endl;
                    }
                }
            }

        }
        else if(len == 0)
        {
            std::cout<<"server closed"<<std::endl;
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }

    close(sockfd);

    return 0;
}