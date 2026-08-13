#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <limits>
#include "Logger.h"
#include "Buffer.h"
#include "MessageCodec.h"
#include "chat.pb.h"
#include "Util.h"

using namespace std;

int currentUserid = -1;
std::ofstream recvFile;
std::string recvFileId;
uint64_t recvFileSize = 0;
std::ofstream downloadFile;
std::string downloadFileId;
std::string downloadFilePath;
bool verifyCodeFinished = false;
bool verifyCodeSuccess = false;

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

void verifyCode(
    int sockfd,
    const std::string& email,
    const std::string& code,
    int scene)
{
    chat::VerifyCodeReq req;

    req.set_email(email);
    req.set_code(code);
    req.set_scene(scene);

    std::string data;
    req.SerializeToString(&data);

    std::string packet =
        MessageCodec::encode(
            chat::VERIFY_CODE_MSG,
            data
        );

    sendAll(
        sockfd,
        packet.data(),
        packet.size()
    );
}

void sendCode(int sockfd, string email, int scene) {
    chat::SendCodeReq req;

    req.set_email(email);
    req.set_scene(scene);

    string data;
    req.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::SEND_CODE_MSG, data);

    sendAll(sockfd, packet.data(), packet.size());
}

void queryFriend(int sockfd)
{
    chat::QueryFriendReq req;

    req.set_msgid(chat::QUERY_FRIEND_MSG);


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

void sendFile(int sockfd, int toid, int groupid, const std::string& path)
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
        std::to_string(
            std::chrono::system_clock::now()
            .time_since_epoch()
            .count());

    //-----------------------------
    // 1. FileStart
    //-----------------------------
    chat::FileStartReq startReq;
    startReq.set_toid(toid);
    startReq.set_filename(filename);
    startReq.set_filesize(filesize);
    startReq.set_fileid(fileid);
    startReq.set_groupid(groupid);

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

                if (res.offlinegroupfiles_size() > 0) {
                    cout << "\n===== 离线群文件消息 =====" << endl;

                    for (int i = 0; i < res.offlinegroupfiles_size(); ++i) {
                        const auto& file = res.offlinegroupfiles(i);

                        cout << "\n========== 离线群文件 ==========" << endl;
                        cout << "群组: " << file.groupname() << endl;
                        cout << "发送者: " << file.fromname()
                            << " (userid=" << file.fromid() << ")" << endl;
                        cout << "文件名: " << file.filename() << endl;
                        cout << "文件大小: " << file.filesize() << " bytes" << endl;
                        cout << "fileid: " << file.fileid() << endl;
                        cout << "================================" << endl;
                    }
                }

                if (res.offlinefriendrequests_size() > 0) {
                    for (const auto& request : res.offlinefriendrequests())
                    {
                        cout << "\n========== 离线好友申请 ==========" << endl;
                        std::cout
                            << "userid："
                            << request.userid()
                            << " username："
                            << request.username()
                            << std::endl;
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

                queryFriend(sockfd);
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
                chat::OneChatNotify notify;

                if (!notify.ParseFromString(body)) {
                    cout << "OneChatNotify parse failed" << endl;
                    continue;
                }


                cout
                <<"收到消息:"
                <<"from="
                <<notify.fromname()
                <<" fromid="
                <<notify.fromid()
                <<" msg="
                <<notify.msg()
                <<endl;
            }


            else if(msgid == chat::GROUP_CHAT_MSG)
            {
                chat::GroupChatNotify notify;

                notify.ParseFromString(body);


                cout
                <<"group "
                <<notify.groupid()
                <<" "
                <<"收到消息:"
                <<"from="
                <<notify.username()
                <<" fromid="
                <<notify.userid()
                <<" msg="
                <<notify.msg()
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


            else if(msgid == chat::CANCEL_ACCOUNT_MSG_ACK)
            {
                chat::CancelAccountRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;

                if(res.err() == 0)
                {
                    close(sockfd);
                    return;
                }
            }

            else if(msgid == chat::REG_MSG_ACK)
            {

                chat::RegisterRes res;
                res.ParseFromString(body);

                cout << "register: " << res.errmsg() << endl;

                if(res.err() == 0)
                {
                    cout << "userid: " << res.userid() << endl;
                }
            }

            else if(msgid == chat::GROUP_NOTIFY_MSG)
            {
                chat::GroupRequest req;

                req.ParseFromString(body);

                cout
                <<"收到加群申请:"
                <<" userid="
                <<req.userid()
                <<" username="
                <<req.username()
                <<" groupid="
                <<req.groupid()
                <<" groupname="
                <<req.groupname()
                <<endl;
            }

            else if(msgid == chat::GROUP_ACCEPT_NOTIFY_MSG)
            {
                chat::GroupAcceptNotify notify;

                notify.ParseFromString(body);

                cout
                <<"加入群成功:"
                <<" groupid="
                <<notify.groupid()
                <<" groupname="
                <<notify.groupname()
                <<endl;
            }

            else if(msgid == chat::QUERY_GROUP_REQ_MSG_ACK)
            {
                chat::QueryGroupReqRes res;

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
                    <<"userid:"
                    <<r.userid()
                    <<" username:"
                    <<r.username()
                    <<" groupid:"
                    <<r.groupid()
                    <<" groupname:"
                    <<r.groupname()
                    <<endl;
                }
            }

            else if(msgid == chat::LEAVE_GROUP_MSG_ACK)
            {
                chat::LeaveGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;

                if(res.err() == 2)       
                {
                    cout << "\nYou are owner.\n";
                    cout << "18. transfer owner\n";
                    cout << "19. dissolve group\n";
                }
            }

            else if(msgid == chat::TRANSFER_OWNER_MSG_ACK)
            {
                chat::TransferOwnerRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }

            else if(msgid == chat::DISSOLVE_GROUP_MSG_ACK)
            {
                chat::DissolveGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }

            else if(msgid == chat::QUERY_GROUP_USER_MSG_ACK)
            {
                chat::QueryGroupUserRes res;

                res.ParseFromString(body);

                cout<<res.errmsg()<<endl;

                for(auto &u : res.users())
                {
                    cout
                    <<"userid:"<<u.userid()
                    <<" username:"<<u.username()
                    <<" role:"<<u.role()
                    <<endl;
                }
            }

            else if(msgid == chat::SET_GROUP_ADMIN_MSG_ACK)
            {
                chat::SetGroupAdminRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }

            else if(msgid == chat::REMOVE_GROUP_ADMIN_MSG_ACK)
            {
                chat::RemoveGroupAdminRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }

            else if(msgid == chat::REMOVE_GROUP_USER_MSG_ACK)
            {
                chat::RemoveGroupUserRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }

            else if (msgid == chat::REMOVE_GROUP_USER_NOTIFY_MSG) {
                chat::RemoveGroupUserNotify notify;
                notify.ParseFromString(body);
                cout << "你已被移出群：" << notify.groupname() << " (groupid = " << notify.groupid() << ")" << endl;
            }

            else if(msgid == chat::REFUSE_GROUP_MSG_ACK)
            {
                chat::RefuseGroupRes res;

                res.ParseFromString(body);

                cout
                <<"refuse group:"
                <<res.errmsg()
                <<endl;
            }

            else if(msgid == chat::REFUSE_GROUP_NOTIFY_MSG)
            {
                chat::RefuseGroupNotify notify;

                notify.ParseFromString(body);

                cout
                <<"你的加群申请被拒绝:"
                <<" groupid="
                <<notify.groupid()
                <<" groupname="
                <<notify.groupname()
                <<endl;
            }


            else if (msgid == chat::GROUP_FILE_NOTIFY_MSG)
            {
                chat::GroupFileNotify notify;

                if (!notify.ParseFromString(body))
                {
                    cout << "GroupFileNotify parse failed" << endl;
                    return;
                }

                cout << "\n========== 收到群文件 ==========" << endl;
                cout << "群组: " << notify.groupname() << endl;
                cout << "发送者: " << notify.fromname()
                    << " (userid=" << notify.fromid() << ")" << endl;
                cout << "文件名: " << notify.filename() << endl;
                cout << "文件大小: " << notify.filesize() << " bytes" << endl;
                cout << "fileid: " << notify.fileid() << endl;
                cout << "================================" << endl;
            }

            else if (msgid == chat::BLACKLIST_ADD_MSG_ACK)
            {
                chat::BlacklistAddRes res;

                res.ParseFromString(body);

                cout << "err:" << res.err()
                    << " "
                    << res.errmsg()
                    << endl;
            }

            else if (msgid == chat::BLACKLIST_REMOVE_MSG_ACK)
            {
                chat::BlacklistRemoveRes res;

                res.ParseFromString(body);

                cout << "err:" << res.err()
                    << " "
                    << res.errmsg()
                    << endl;
            }


            else if(msgid == chat::SEND_CODE_MSG_ACK)
            {

                chat::SendCodeRes res;

                res.ParseFromString(body);


                cout
                <<"send code:"
                <<res.errmsg()
                <<endl;

            }

            else if(msgid == chat::VERIFY_CODE_MSG_ACK)
            {
                chat::VerifyCodeRes res;

                res.ParseFromString(body);

                cout << res.errmsg() << endl;

                if(res.err() == 0)
                {
                    verifyCodeSuccess = true;
                }
                else
                {
                    verifyCodeSuccess = false;
                }

                verifyCodeFinished = true;
            }

            else if (msgid == chat::CODE_LOGIN_MSG_ACK)
            {
                chat::CodeLoginRes res;

                res.ParseFromString(body);

                if (res.err() != 0)
                {
                    cout << "登录失败：" << res.errmsg() << endl;
                    continue;
                }

                cout << "登录成功" << endl;
                cout << "userid: " << res.userid() << endl;
                cout << "username: " << res.name() << endl;
            }

            else if (msgid == chat::RESET_PASSWORD_MSG_ACK)
            {
                chat::ResetPasswordRes res;

                res.ParseFromString(body);

                cout << "reset password: "
                    << res.errmsg()
                    << endl;
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
        cout<<"12 cancel account\n";
        cout<<"13 register\n";
        cout<<"14 apply group\n";
        cout<<"15 query group request\n";
        cout<<"16 accept group\n";
        cout<<"17 leave group\n";
        cout<<"18 transfer owner\n";
        cout<<"19 dissolve group\n";
        cout<<"20 query group users\n";
        cout<<"21 set group admin\n";
        cout<<"22 remove group admin\n";
        cout<<"23 remove group user\n";
        cout<<"24 refuse group\n";
        cout<<"25 send group file\n";
        cout<<"26 add blacklist\n";
        cout<<"27 remove blacklist\n";
        cout<<"28 login by code\n";
        cout<<"29 reset password\n";
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
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            pwd = inputPassword();



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

            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            chat::AddFriendReq req;

            int toid;


            cout<<"toid:";
            cin>>toid;

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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            chat::QueryFriendReqReq req;

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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            chat::AcceptFriendReq req;


            int friendid;

            cout<<"friendid:";
            cin>>friendid;


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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            chat::DeleteFriendReq req;

            int friendid;


            cout<<"friendid:";
            cin>>friendid;


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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            chat::OneChatReq req;

            int toid;
            string msg;


          


            cout<<"toid:";
            cin>>toid;


            cout<<"msg:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, msg);

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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            chat::HistoryMsgReq req;


            int friendid;

            cout<<"friendid:";
            cin>>friendid;

            req.set_friendid(friendid);



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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            chat::GroupChatReq req;

            int groupid;
            string msg;


            cout<<"groupid:";
            cin>>groupid;

            cout<<"msg:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, msg);


            req.set_groupid(groupid);
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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
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
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            int toid;
            string path;

            cout<<"toid:";
            cin>>toid;

            cout<<"file path:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin,path);


            thread t(
                sendFile,
                sockfd,
                toid,
                0,
                path
            );


            t.detach();
        }

        else if(op == 11)
        {
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }
            string fileid;

            cout << "fileid:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, fileid);

            chat::DownloadFileReq req;

            req.set_fileid(fileid);

            req.SerializeToString(
                &data
            );


            packet =
            MessageCodec::encode(
                chat::DOWNLOAD_FILE_MSG,
                data
            );

        }

        else if(op == 12)
        {
            if(currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }

            chat::CancelAccountReq req;

            req.SerializeToString(
                &data
            );


            packet =
            MessageCodec::encode(
                chat::CANCEL_ACCOUNT_MSG,
                data
            );

        }

        else if(op == 13)
        {

            chat::RegisterReq req;

            string email = "";

            
            cout<<"email:";
            cin>>email;

            sendCode(sockfd, email, 1);
            cout<<"验证码已发送" << endl;

            string code;
            cout<<"code:";
            cin>>code;

            verifyCodeFinished = false;
            verifyCodeSuccess = false;

            verifyCode(
                sockfd,
                email,
                code,
                1
            );

            while(!verifyCodeFinished)
            {
                usleep(10000);
            }

            if(!verifyCodeSuccess)
            {
                cout << "验证码错误，注册结束" << endl;
                continue;
            }

            cout << "验证码正确，请继续输入用户名和密码" << endl;

            string name;
            string pwd;

            cout << "username:";
            cin >> name;

            cout << "password:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            pwd = inputPassword();

            req.set_name(name);
            req.set_password(pwd);
            req.set_email(email);

            req.SerializeToString(&data);

            packet = MessageCodec::encode(chat::REG_MSG, data);

        }

        else if(op==14)
        {
            chat::ApplyGroupReq req;

            int groupid;


            cout<<"groupid:";
            cin>>groupid;

            req.set_groupid(groupid);


            req.SerializeToString(&data);


            packet =
            MessageCodec::encode(
                chat::APPLY_GROUP_MSG,
                data
            );
        }

        else if(op==15)
        {
            chat::QueryGroupReqReq req;


            int groupid;

            cout<<"groupid:";
            cin>>groupid;


            req.set_groupid(groupid);


            req.SerializeToString(&data);


            packet =
            MessageCodec::encode(
                chat::QUERY_GROUP_REQ_MSG,
                data
            );
        }

        else if(op==16)
        {
            chat::AcceptGroupReq req;


            int groupid;


            cout<<"groupid:";
            cin>>groupid;

            req.set_groupid(groupid);


            req.SerializeToString(&data);


            packet =
            MessageCodec::encode(
                chat::ACCEPT_GROUP_MSG,
                data
            );
        }

        else if(op == 17)
        {
            chat::LeaveGroupReq req;

           
            int groupid;

            cout << "groupid:";
            cin >> groupid;

            req.set_groupid(groupid);

            req.SerializeToString(&data);

            packet = MessageCodec::encode(
                chat::LEAVE_GROUP_MSG,
                data
            );
        }

        else if(op == 18)
        {
            chat::TransferOwnerReq req;

            int newownerid;
            int groupid;


            cout << "new owner:";
            cin >> newownerid;

            cout << "groupid:";
            cin >> groupid;

            req.set_newownerid(newownerid);
            req.set_groupid(groupid);

            req.SerializeToString(&data);

            packet = MessageCodec::encode(
                chat::TRANSFER_OWNER_MSG,
                data
            );
        }

        else if(op == 19)
        {
            chat::DissolveGroupReq req;

            int groupid;

            cout << "groupid:";
            cin >> groupid;

            req.set_groupid(groupid);

            req.SerializeToString(&data);

            packet = MessageCodec::encode(
                chat::DISSOLVE_GROUP_MSG,
                data
            );
        }

        else if (op == 20) {
            chat::QueryGroupUserReq req;

            int groupid;

            cout<<"groupid:";
            cin>>groupid;

            req.set_groupid(groupid);

            req.SerializeToString(&data);

            packet =
            MessageCodec::encode(
                chat::QUERY_GROUP_USER_MSG,
                data
            );
        }

        else if (op == 21) {
            chat::SetGroupAdminReq req;

            int groupid;
            int targetid;

            cout<<"groupid:";
            cin>>groupid;

            cout<<"targetid:";
            cin>>targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);

            req.SerializeToString(&data);

            packet =
            MessageCodec::encode(
                chat::SET_GROUP_ADMIN_MSG,
                data
            );
        }

        else if (op == 22) {
            chat::RemoveGroupAdminReq req;

            int groupid;
            int targetid;

            cout<<"groupid:";
            cin>>groupid;

            cout<<"targetid:";
            cin>>targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);

            req.SerializeToString(&data);

            packet =
            MessageCodec::encode(
                chat::REMOVE_GROUP_ADMIN_MSG,
                data
            );
        }

        else if (op == 23) {
            chat::RemoveGroupUserReq req;

            int groupid;
            int targetid;

            cout<<"groupid:";
            cin>>groupid;

            cout<<"targetid:";
            cin>>targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);

            req.SerializeToString(&data);

            packet =
            MessageCodec::encode(
                chat::REMOVE_GROUP_USER_MSG,
                data
            );
        }

        else if(op == 24)
        {
            chat::RefuseGroupReq req;

            int groupid;
            int targetid;

            cout << "groupid:";
            cin >> groupid;

            cout << "targetid:";
            cin >> targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);

            req.SerializeToString(&data);

            packet =
                MessageCodec::encode(
                    chat::REFUSE_GROUP_MSG,
                    data
                );
        }

        else if (op == 25) {
            int groupid;
            string filepath;

            cout << "groupid:";
            cin >> groupid;

            cout << "file path:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, filepath);

            thread t(
                sendFile,
                sockfd,
                0,
                groupid,
                filepath
            );

            t.detach();
        }

        else if (op == 26)
        {
            if (currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }

            int blackid;

            cout << "blackid:";
            cin >> blackid;

            chat::BlacklistAddReq req;
            req.set_blackid(blackid);

            std::string data;
            req.SerializeToString(&data);

            std::string packet =
                MessageCodec::encode(
                    chat::BLACKLIST_ADD_MSG,
                    data
                );

            sendAll(sockfd, packet.data(), packet.size());
        }

        else if (op == 27)
        {
            if (currentUserid == -1)
            {
                cout << "please login first" << endl;
                continue;
            }

            int blackid;

            cout << "blackid:";
            cin >> blackid;

            chat::BlacklistRemoveReq req;
            req.set_blackid(blackid);

            std::string data;
            req.SerializeToString(&data);

            std::string packet =
                MessageCodec::encode(
                    chat::BLACKLIST_REMOVE_MSG,
                    data
                );

            sendAll(sockfd, packet.data(), packet.size());
        }

        else if(op==28)
        {

            chat::CodeLoginReq req;

            string email = "";

            
            cout<<"email:";
            cin>>email;

            sendCode(sockfd, email, 2);
            cout<<"验证码已发送" << endl;

            string code;
            cout<<"code:";
            cin>>code;

            verifyCodeFinished = false;
            verifyCodeSuccess = false;

            verifyCode(
                sockfd,
                email,
                code,
                2
            );

            while(!verifyCodeFinished)
            {
                usleep(10000);
            }

            if(!verifyCodeSuccess)
            {
                cout << "验证码错误，登录结束" << endl;
                continue;
            }

            cout << "验证码正确，正在登录..." << endl;

            req.set_email(email);
            req.set_code(code);
            req.SerializeToString(&data);

            packet = MessageCodec::encode(chat::CODE_LOGIN_MSG, data);

        }

        else if (op == 29)
        {
            string email;

            cout << "email:";
            cin >> email;

            sendCode(sockfd, email, 3);

            cout << "验证码已发送" << endl;

            string code;

            cout << "code:";
            cin >> code;

            verifyCodeFinished = false;
            verifyCodeSuccess = false;

            verifyCode(
                sockfd,
                email,
                code,
                3
            );

            while (!verifyCodeFinished)
            {
                usleep(10000);
            }

            if (!verifyCodeSuccess)
            {
                cout << "验证码错误，密码找回结束" << endl;
                continue;
            }

            cout << "验证码正确，请输入新密码" << endl;

            string password;
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            password = inputPassword();

            chat::ResetPasswordReq req;

            //
            cout << "email = [" << email << "]" << endl;
            cout << "password = [" << password << "]" << endl;
            cout << "password size = " << password.size() << endl;
            
            req.set_email(email);
            req.set_newpassword(password);

            req.SerializeToString(&data);

            packet = MessageCodec::encode(
                chat::RESET_PASSWORD_MSG,
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