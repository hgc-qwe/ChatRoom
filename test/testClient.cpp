#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include "Buffer.h"
#include "MessageCodec.h"
#include "chat.pb.h"

using namespace std;


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



        if(op==1)
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



            req.SerializeToString(&data);


            packet =
                MessageCodec::encode(
                    chat::LOGIN_MSG,
                    data
                );
        }



        else if(op==2)
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


            req.SerializeToString(&data);



            packet =
                MessageCodec::encode(
                    chat::ADD_FRIEND_MSG,
                    data
                );
        }




        else if(op==3)
        {
            chat::QueryFriendReqReq req;


            int userid;


            cout<<"userid:";
            cin>>userid;


            req.set_userid(userid);



            req.SerializeToString(&data);


            packet =
                MessageCodec::encode(
                    chat::QUERY_FRIEND_REQ_MSG,
                    data
                );

        }




        else if(op==4)
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


            req.SerializeToString(&data);



            packet =
                MessageCodec::encode(
                    chat::ACCEPT_FRIEND_MSG,
                    data
                );
        }



        else if(op==0)
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



        cout<<"send bytes:"
            <<packet.size()
            <<endl;



        char buffer[4096]={0};


        int n =
            recv(
                sockfd,
                buffer,
                sizeof(buffer),
                0
            );


        if(n<=0)
        {
            cout<<"server close"<<endl;
            break;
        }

        Buffer buf;
        buf.append(buffer, n);



        int msgid;
        string body;


        MessageCodec::decode(
            buf,
            msgid,
            body
        );



        cout<<"recv msgid:"
            <<msgid
            <<endl;



        if(msgid==chat::LOGIN_MSG_ACK)
        {
            chat::LoginRes res;

            res.ParseFromString(body);


            cout
            <<"err:"
            <<res.err()
            <<" "
            <<res.errmsg()
            <<endl;
        }


        else if(msgid==chat::ADD_FRIEND_MSG_ACK)
        {
            chat::AddFriendRes res;

            res.ParseFromString(body);


            cout
            <<"err:"
            <<res.err()
            <<" "
            <<res.errmsg()
            <<endl;
        }



        else if(msgid==chat::QUERY_FRIEND_REQ_MSG_ACK)
        {
            chat::QueryFriendReqRes res;


            res.ParseFromString(body);



            cout
            <<"err:"
            <<res.err()
            <<" "
            <<res.errmsg()
            <<endl;



            for(auto& r:res.requests())
            {
                cout
                <<"request userid:"
                <<r.userid()
                <<" name:"
                <<r.username()
                <<endl;
            }
        }



        else if(msgid==chat::ACCEPT_FRIEND_MSG_ACK)
        {
            chat::AcceptFriendRes res;


            res.ParseFromString(body);



            cout
            <<"err:"
            <<res.err()
            <<" "
            <<res.errmsg()
            <<endl;
        }

    }


    return 0;
}