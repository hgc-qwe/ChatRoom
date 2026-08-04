#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include "Buffer.h"
#include "MessageCodec.h"
#include "chat.pb.h"

using namespace std;

int currentUserid = -1;
std::ofstream recvFile;
std::string recvFileId;
uint64_t recvFileSize = 0;
std::ofstream downloadFile;
std::string downloadFileId;
std::string downloadFilePath;

void queryFriend(int sockfd, int userid)
{
    chat::QueryFriendReq req;

    req.set_userid(userid);


    string data;

    req.SerializeToString(&data);


    string packet =
        MessageCodec::encode(
            chat::QUERY_FRIEND_MSG,
            data
        );


    send(
        sockfd,
        packet.data(),
        packet.size(),
        0
    );
}

bool sendAll(int sockfd,
             const char* data,
             size_t len)
{
    size_t sent = 0;

    while(sent < len)
    {
        int n = send(
            sockfd,
            data + sent,
            len - sent,
            0
        );


        if(n <= 0)
            return false;


        sent += n;
    }


    return true;
}

void sendFile(int sockfd,
              int fromid,
              int toid,
              const std::string& path)
{
    namespace fs = std::filesystem;

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
    {
        std::cout << "open file failed\n";
        return;
    }

    // 文件大小
    uint64_t filesize = fs::file_size(path);

    // 文件名
    std::string filename = fs::path(path).filename().string();

    // 生成一个简单 fileid
    std::string fileid =
        std::to_string(fromid) + "_" +
        std::to_string(
            std::chrono::system_clock::now()
            .time_since_epoch()
            .count());

    //-----------------------------
    // 1. FileStart
    //-----------------------------
    chat::FileStartReq startReq;
    startReq.set_fromid(fromid);
    startReq.set_toid(toid);
    startReq.set_filename(filename);
    startReq.set_filesize(filesize);
    startReq.set_fileid(fileid);

    std::string data;
    startReq.SerializeToString(&data);

    std::string packet =
        MessageCodec::encode(
            chat::FILE_START_MSG,
            data);

    sendAll(sockfd,
         packet.data(),
         packet.size());

    //-----------------------------
    // 2. FileChunk
    //-----------------------------
    const size_t CHUNK_SIZE = 1024 * 1024;   // 64KB

    char buffer[CHUNK_SIZE];
    uint64_t offset = 0;

    while (ifs)
    {
        ifs.read(buffer, CHUNK_SIZE);

        std::streamsize len = ifs.gcount();

        if (len <= 0)
            break;

        chat::FileChunkReq chunkReq;
        chunkReq.set_fileid(fileid);
        chunkReq.set_offset(offset);
        chunkReq.set_data(buffer, len);

        data.clear();
        chunkReq.SerializeToString(&data);

        packet =
            MessageCodec::encode(
                chat::FILE_CHUNK_MSG,
                data);

        sendAll(sockfd,
             packet.data(),
             packet.size());

        offset += len;
    }

    //-----------------------------
    // 3. FileEnd
    //-----------------------------
    chat::FileEndReq endReq;
    endReq.set_fileid(fileid);

    data.clear();
    endReq.SerializeToString(&data);

    packet =
        MessageCodec::encode(
            chat::FILE_END_MSG,
            data);

    sendAll(sockfd,
         packet.data(),
         packet.size());

    std::cout << "file send finish\n";
}


// 接收服务器消息线程
void recvMessage(int sockfd)
{
    Buffer buf;

    while(true)
    {
        char buffer[64 * 1024] = {0};


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
                    << "login:"
                    << res.errmsg()
                    << endl;

                // 私聊离线消息
                if(res.offlinemsgs_size() > 0)
                {
                    cout << "\n===== 离线私聊消息 =====" << endl;

                    for(auto &msg : res.offlinemsgs())
                    {
                        cout
                            << "[" << msg.time() << "] "
                            << msg.fromid()
                            << " -> "
                            << msg.toid()
                            << " : "
                            << msg.msg()
                            << endl;
                    }
                }

                // 群聊离线消息
                if(res.offlinegroupmsg_size() > 0)
                {
                    cout << "\n===== 离线群聊消息 =====" << endl;

                    for(auto &msg : res.offlinegroupmsg())
                    {
                        cout
                            << "[" << msg.time() << "] "
                            << "group "
                            << msg.groupid()
                            << " user "
                            << msg.userid()
                            << " : "
                            << msg.msg()
                            << endl;
                    }
                }

                if(res.offlinefiles_size() > 0)
                {
                    cout << "\n===== 离线文件消息 =====" << endl;

                    for(auto& file : res.offlinefiles())
                    {
                        cout
                        <<"离线文件:"
                        <<file.filename()
                        <<" size:"
                        <<file.filesize()
                        <<" from:"
                        <<file.fromid()
                        <<" fileid:"
                        <<file.fileid()
                        <<endl;
                    }
                }
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

                cout << endl;

                queryFriend(sockfd, currentUserid);
            }

            else if(msgid == chat::QUERY_FRIEND_MSG_ACK)
            {
                chat::QueryFriendRes res;

                res.ParseFromString(body);


                cout
                <<"err:"
                <<res.err()
                <<" "
                <<res.errmsg()
                <<endl;


                for(auto& user : res.friends())
                {
                    cout
                    <<"friend id:"
                    <<user.id()
                    <<" name:"
                    <<user.name()
                    <<" state:"
                    <<user.state()
                    <<endl;
                }
            }


            else if(msgid==chat::DELETE_FRIEND_MSG_ACK)
            {
                chat::DeleteFriendRes res;

                res.ParseFromString(body);


                cout
                <<"delete:"
                <<res.errmsg()
                <<endl;
            }


            else if(msgid == chat::HISTORY_MSG_ACK)
            {
                chat::HistoryMsgRes res;

                res.ParseFromString(body);


                cout
                <<"err:"
                <<res.err()
                <<" "
                <<res.errmsg()
                <<endl;


                for(auto& msg : res.msgs())
                {
                    cout
                    <<"from:"
                    <<msg.fromid()
                    <<" -> "
                    <<"to:"
                    <<msg.toid()
                    <<" msg:"
                    <<msg.msg()
                    <<right
                    <<setw(10)
                    <<"time:"
                    <<msg.time()
                    <<endl;
                }
            }
            

            else if(msgid == chat::ONE_CHAT_MSG)
            {
                chat::OneChatReq req;

                req.ParseFromString(body);


                cout
                <<"收到消息:"
                <<"from="
                <<req.fromid()
                <<" msg="
                <<req.msg()
                <<endl;
            }


            else if(msgid == chat::GROUP_CHAT_MSG)
            {
                chat::GroupChatReq req;

                req.ParseFromString(body);


                cout
                <<"group "
                <<req.groupid()
                <<" "
                <<"收到消息:"
                <<"from user="
                <<req.userid()
                <<" msg="
                <<req.msg()
                <<endl;
            }

            else if(msgid == chat::GROUP_HISTORY_MSG_ACK)
            {
                chat::GroupHistoryMsgRes res;

                res.ParseFromString(body);


                cout
                <<"err:"
                <<res.err()
                <<" "
                <<res.errmsg()
                <<endl;


                for(auto& msg : res.msgs())
                {
                    cout
                    <<"group:"
                    <<msg.groupid()
                    <<"     "
                    <<"user:"
                    <<msg.userid()
                    <<" msg:"
                    <<msg.msg()
                    <<right
                    <<setw(10)
                    <<"time:"
                    <<msg.time()
                    <<endl;
                }
            }

            else if(msgid == chat::FILE_START_MSG)
            {
                chat::FileStartReq req;

                req.ParseFromString(body);


                cout
                <<"收到文件:"
                <<req.filename()
                <<" size:"
                <<req.filesize()
                <<endl;



                std::filesystem::create_directory(
                    "./clientDownload"
                );


                std::string path =
                    "./clientDownload/"
                    + req.filename();


                recvFile.open(
                    path,
                    std::ios::binary
                );


                if(recvFile.is_open())
                {
                    recvFileId = req.fileid();

                    recvFileSize=req.filesize();

                    cout
                    <<"开始接收文件:"
                    <<path
                    <<endl;
                }
                else
                {
                    cout<<"open recv file failed"<<endl;
                }

            }

            else if(msgid == chat::FILE_CHUNK_MSG)
            {
                chat::FileChunkReq req;


                req.ParseFromString(body);



                if(recvFile.is_open()
                && req.fileid()==recvFileId)
                {

                    recvFile.write(
                        req.data().data(),
                        req.data().size()
                    );


                }

            }


            else if(msgid == chat::FILE_END_MSG)
            {

                chat::FileEndReq req;


                req.ParseFromString(body);



                if(recvFile.is_open()
                && req.fileid()==recvFileId)
                {

                    recvFile.close();


                    cout
                    <<"文件接收完成"
                    <<endl;

                }

            }

            else if(msgid == chat::DOWNLOAD_START_MSG)
            {
                chat::DownloadStart start;

                start.ParseFromString(body);


                cout
                <<"开始下载:"
                <<start.filename()
                <<" size:"
                <<start.filesize()
                <<endl;


                // 创建下载目录
                std::filesystem::create_directory(
                    "./clientDownload"
                );


                // 保存路径
                downloadFilePath =
                    "./clientDownload/"
                    + start.filename();


                downloadFile.open(
                    downloadFilePath,
                    std::ios::binary
                );


                if(downloadFile.is_open())
                {
                    downloadFileId = start.fileid();


                    cout
                    <<"保存到:"
                    <<downloadFilePath
                    <<endl;
                }
                else
                {
                    cout
                    <<"open download file failed"
                    <<endl;
                }
            }

            else if(msgid == chat::DOWNLOAD_CHUNK_MSG)
            {
                chat::DownloadChunk chunk;

                chunk.ParseFromString(body);


                if(downloadFile.is_open()
                && chunk.fileid()==downloadFileId)
                {

                    downloadFile.write(
                        chunk.data().data(),
                        chunk.data().size()
                    );


                    cout
                    <<"receive:"
                    <<chunk.data().size()
                    <<" bytes"
                    <<endl;
                }
            }


            else if(msgid == chat::DOWNLOAD_END_MSG)
            {
                chat::DownloadEnd end;

                end.ParseFromString(body);


                if(downloadFile.is_open()
                && end.fileid()==downloadFileId)
                {
                    downloadFile.close();


                    cout
                    <<"下载完成:"
                    <<downloadFilePath
                    <<endl;
                }
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
        cout<<"5 delete friend\n";
        cout<<"6 one chat\n";
        cout<<"7 query history msg\n";
        cout<<"8 group chat\n";
        cout<<"9 query group history msg\n";
        cout<<"10 send file\n";
        cout<<"11 download file\n";
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
            currentUserid = id;

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

        else if(op==5)
        {
            chat::DeleteFriendReq req;


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
                    chat::DELETE_FRIEND_MSG,
                    data
                );
        }

        else if(op == 6)
        {
            chat::OneChatReq req;

            int fromid;
            int toid;
            string msg;


            cout<<"fromid:";
            cin>>fromid;


            cout<<"toid:";
            cin>>toid;


            cout<<"msg:";
            cin.ignore();
            getline(cin, msg);


            req.set_fromid(fromid);
            req.set_toid(toid);
            req.set_msg(msg);


            req.SerializeToString(&data);


            packet =
            MessageCodec::encode(
                chat::ONE_CHAT_MSG,
                data
            );
        }

        else if(op == 7)
        {

            chat::HistoryMsgReq req;


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
                chat::HISTORY_MSG,
                data
            );

        }


        else if(op == 8)
        {
            chat::GroupChatReq req;

            int groupid;
            int userid;
            string msg;


            cout<<"groupid:";
            cin>>groupid;


            cout<<"userid:";
            cin>>userid;


            cout<<"msg:";
            cin.ignore();
            getline(cin, msg);


            req.set_groupid(groupid);
            req.set_userid(userid);
            req.set_msg(msg);


            req.SerializeToString(&data);


            packet =
            MessageCodec::encode(
                chat::GROUP_CHAT_MSG,
                data
            );
        }

        else if(op == 9)
        {

            chat::GroupHistoryMsgReq req;


            int groupid;


            cout<<"groupid:";
            cin>>groupid;

            



            req.set_groupid(groupid);




            req.SerializeToString(
                &data
            );


            packet =
            MessageCodec::encode(
                chat::GROUP_HISTORY_MSG,
                data
            );

        }

        else if(op == 10)
        {
            int fromid,toid;
            string path;


            cout<<"fromid:";
            cin>>fromid;

            cout<<"toid:";
            cin>>toid;

            cout<<"file path:";
            cin.ignore();
            getline(cin,path);


            thread t(
                sendFile,
                sockfd,
                fromid,
                toid,
                path
            );


            t.detach();
        }

        else if(op == 11)
        {
            
            string fileid;

            cout << "fileid:";
            cin.ignore();
            getline(cin, fileid);

            chat::DownloadFileReq req;

            req.set_fileid(fileid);
            req.set_userid(currentUserid);

            req.SerializeToString(
                &data
            );


            packet =
            MessageCodec::encode(
                chat::DOWNLOAD_FILE_MSG,
                data
            );

        }

        else
        {
            continue;
        }



        sendAll(sockfd, packet.data(), packet.size());


        cout
        <<"send bytes:"
        <<packet.size()
        <<endl;

    }



    return 0;
}