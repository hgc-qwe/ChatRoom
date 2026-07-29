#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

#include "Buffer.h"
#include "MessageCodec.h"
#include "chat.pb.h"

using namespace std;


// 接收服务器消息线程
void recvMessage(int sockfd)
{
    while(true)
    {
        char buffer[4096] = {0};


        int n = recv(
            sockfd,
            buffer,
            sizeof(buffer),
            0
        );


        if(n <= 0)
        {
            cout<<"server close"<<endl;
            break;
        }


        Buffer buf;

        buf.append(
            buffer,
            n
        );


        int msgid;
        string body;


        while(
            MessageCodec::decode(
                buf,
                msgid,
                body
            )
        )
        {

            cout<<"\nrecv msgid:"
                <<msgid
                <<endl;



            // 登录响应
            if(msgid == chat::LOGIN_MSG_ACK)
            {
                chat::LoginRes res;

                res.ParseFromString(body);


                cout
                <<"login:"
                <<res.errmsg()
                <<endl;
            }


            // 添加好友响应
            else if(msgid == chat::ADD_FRIEND_MSG_ACK)
            {
                chat::AddFriendRes res;

                res.ParseFromString(body);


                cout
                <<"add friend:"
                <<res.errmsg()
                <<endl;
            }


            // 查询好友申请响应
            else if(msgid == chat::QUERY_FRIEND_REQ_MSG_ACK)
            {
                chat::QueryFriendReqRes res;

                res.ParseFromString(body);


                cout
                <<"err:"
                <<res.err()
                <<" "
                <<res.errmsg()
                <<endl;


                for(auto& r : res.requests())
                {
                    cout
                    <<"request userid:"
                    <<r.userid()
                    <<" username:"
                    <<r.username()
                    <<endl;
                }
            }


            // 同意好友响应
            else if(msgid == chat::ACCEPT_FRIEND_MSG_ACK)
            {
                chat::AcceptFriendRes res;

                res.ParseFromString(body);


                cout
                <<"accept:"
                <<res.errmsg()
                <<endl;
            }



            // 好友申请实时通知
            else if(msgid == chat::FRIEND_NOTIFY_MSG)
            {
                chat::FriendRequest req;


                req.ParseFromString(body);


                cout
                <<"收到好友申请:"
                <<" userid="
                <<req.userid()
                <<" username="
                <<req.username()
                <<endl;
            }

            else if(msgid == chat::FRIEND_ACCEPT_NOTIFY_MSG)
            {
                chat::FriendAcceptNotify req;


                req.ParseFromString(body);


                cout
                <<"好友申请通过:"
                <<" userid="
                <<req.userid()
                <<" username="
                <<req.username()
                <<endl;
            }


            cout<<"\n";
        }

    }
}



int main()
{

    int sockfd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );


    sockaddr_in server;


    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr =
        inet_addr("127.0.0.1");



    if(connect(
        sockfd,
        (sockaddr*)&server,
        sizeof(server)
    ) < 0)
    {
        cout<<"connect failed"<<endl;
        return -1;
    }


    cout<<"connect success"<<endl;



    // 开启接收线程
    thread recvThread(
        recvMessage,
        sockfd
    );


    recvThread.detach();



    while(true)
    {

        cout<<"\n====== menu ======\n";
        cout<<"1 login\n";
        cout<<"2 add friend\n";
        cout<<"3 query friend request\n";
        cout<<"4 accept friend\n";
        cout<<"0 exit\n";


        int op;

        cin>>op;



        string data;
        string packet;



        // 登录
        if(op == 1)
        {

            chat::LoginReq req;


            int id;
            string pwd;


            cout<<"id:";
            cin>>id;


            cout<<"password:";
            cin>>pwd;



            req.set_id(id);
            req.set_password(pwd);



            req.SerializeToString(
                &data
            );


            packet =
            MessageCodec::encode(
                chat::LOGIN_MSG,
                data
            );

        }



        // 添加好友
        else if(op == 2)
        {

            chat::AddFriendReq req;


            int fromid;
            int toid;


            cout<<"fromid:";
            cin>>fromid;


            cout<<"toid:";
            cin>>toid;



            req.set_fromid(fromid);

            req.set_toid(toid);



            req.SerializeToString(
                &data
            );



            packet =
            MessageCodec::encode(
                chat::ADD_FRIEND_MSG,
                data
            );

        }



        // 查询好友申请
        else if(op == 3)
        {

            chat::QueryFriendReqReq req;


            int userid;


            cout<<"userid:";
            cin>>userid;



            req.set_userid(userid);



            req.SerializeToString(
                &data
            );


            packet =
            MessageCodec::encode(
                chat::QUERY_FRIEND_REQ_MSG,
                data
            );

        }



        // 同意好友
        else if(op == 4)
        {

            chat::AcceptFriendReq req;


            int userid;
            int friendid;



            cout<<"userid:";
            cin>>userid;


            cout<<"friendid:";
            cin>>friendid;



            req.set_userid(userid);

            req.set_friendid(friendid);



            req.SerializeToString(
                &data
            );



            packet =
            MessageCodec::encode(
                chat::ACCEPT_FRIEND_MSG,
                data
            );

        }



        else if(op == 0)
        {
            close(sockfd);
            break;
        }


        else
        {
            continue;
        }



        send(
            sockfd,
            packet.data(),
            packet.size(),
            0
        );


        cout
        <<"send bytes:"
        <<packet.size()
        <<endl;

    }



    return 0;
}