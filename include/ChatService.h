#pragma once
#include <unordered_map>
#include <mutex>
#include <cstdio>
#include "Redis.h"
#include "UserModel.h"
#include "FriendModel.h"
#include "GroupModel.h"
#include "chat.pb.h"
#include "MessageModel.h"
#include "TcpConnection.h"
#include "FriendReqModel.h"
#include "GroupMessageModel.h"
#include "FileModel.h"

struct FileInfo {
    int fromid;
    int toid;
    std::string filename;
    std::string fileid;
    uint64_t filesize;
    std::string path;
};

class ChatService
{
public:
    static ChatService* instance();

    void login(std::shared_ptr<TcpConnection> conn, const chat::LoginReq& req, chat::LoginRes& res);
    void reg(std::shared_ptr<TcpConnection> conn, const chat::RegisterReq& req, chat::RegisterRes& res);

    void addFriend(std::shared_ptr<TcpConnection> conn, const chat::AddFriendReq& req, chat::AddFriendRes& res);

    void oneChat(std::shared_ptr<TcpConnection> conn, const chat::OneChatReq& req, chat::OneChatRes& res);

    void createGroup(std::shared_ptr<TcpConnection> conn, const chat::CreateGroupReq& req, chat::CreateGroupRes& res);
    void addGroup(std::shared_ptr<TcpConnection> conn, const chat::AddGroupReq& req, chat::AddGroupRes& res);

    void groupChat(std::shared_ptr<TcpConnection> conn, const chat::GroupChatReq& req, chat::GroupChatRes& res);

    void logout(std::shared_ptr<TcpConnection> conn, const chat::LogoutReq& req, chat::LogoutRes& res);

    void queryFriend(std::shared_ptr<TcpConnection> conn, const chat::QueryFriendReq& req, chat::QueryFriendRes& res);

    void acceptFriend(std::shared_ptr<TcpConnection> conn, const chat::AcceptFriendReq& req, chat::AcceptFriendRes& res);

    void queryFriendreq(std::shared_ptr<TcpConnection> conn, const chat::QueryFriendReqReq& req, chat::QueryFriendReqRes& res);
    
    void deleteFriend(std::shared_ptr<TcpConnection> conn, const chat::DeleteFriendReq& req, chat::DeleteFriendRes& res);

    void queryHistoryMsg(std::shared_ptr<TcpConnection> conn, const chat::HistoryMsgReq& req, chat::HistoryMsgRes& res);

    void queryGroupHistoryMsg(std::shared_ptr<TcpConnection> conn, const chat::GroupHistoryMsgReq& req, chat::GroupHistoryMsgRes& res);

    void fileStart(std::shared_ptr<TcpConnection> conn, const chat::FileStartReq& req, chat::FileStartRes& res);

    void fileChunk(std::shared_ptr<TcpConnection> conn, const chat::FileChunkReq& req, chat::FileChunkRes& res);

    void fileEnd(std::shared_ptr<TcpConnection> conn, const chat::FileEndReq& req, chat::FileEndRes& res);

    void sendFile(std::shared_ptr<TcpConnection> conn, const FileInfo& info);

    void downloadFile(std::shared_ptr<TcpConnection> conn, const chat::DownloadFileReq& req, chat::DownloadFileRes& res);

    void cancelAccount(std::shared_ptr<TcpConnection> conn, const chat::CancelAccountReq& req, chat::CancelAccountRes& res);

    void addUserConn(std::shared_ptr<TcpConnection> conn, int userid);

    std::shared_ptr<TcpConnection> getUserConn(int userid);

    void removeUser(int userid);
    void clientClose(int userid);
private:
    UserModel userModel;
    FriendModel friendModel;
    GroupModel groupModel;
    MessageModel messageModel;
    FriendReqModel friendReqModel;
    GroupMessageModel groupMessageModel;
    FileModel fileModel;
    Redis redis;

    std::unordered_map<std::string, FILE*> fileMap;
    std::unordered_map<std::string, FileInfo> fileInfoMap;
    std::mutex fileMutex;

    std::unordered_map<int, std::shared_ptr<TcpConnection>> userConnMap;
    std::mutex connMutex;

    ChatService() {
        redis.connect();
    }
};