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
#include "GroupReqModel.h"
#include "FileModel.h"
#include "BlacklistModel.h"

struct FileInfo {
    int fromid;
    int toid;
    int groupid;
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

    void applyGroup(std::shared_ptr<TcpConnection> conn, const chat::ApplyGroupReq& req, chat::ApplyGroupRes& res);

    void queryGroupreq(std::shared_ptr<TcpConnection> conn, const chat::QueryGroupReqReq& req, chat::QueryGroupReqRes& res);

    void acceptGroup(std::shared_ptr<TcpConnection> conn, const chat::AcceptGroupReq& req, chat::AcceptGroupRes& res);

    void queryGroup(std::shared_ptr<TcpConnection> conn, const chat::QueryGroupReq& req, chat::QueryGroupRes& res);

    void leaveGroup(std::shared_ptr<TcpConnection> conn, const chat::LeaveGroupReq& req, chat::LeaveGroupRes& res);

    void transferOwner(std::shared_ptr<TcpConnection> conn, const chat::TransferOwnerReq& req, chat::TransferOwnerRes& res);

    void dissolveGroup(std::shared_ptr<TcpConnection> conn, const chat::DissolveGroupReq& req, chat::DissolveGroupRes& res);

    void queryGroupUsers(std::shared_ptr<TcpConnection> conn, const chat::QueryGroupUserReq& req, chat::QueryGroupUserRes& res);

    void setGroupAdmin(std::shared_ptr<TcpConnection> conn, const chat::SetGroupAdminReq& req, chat::SetGroupAdminRes& res);

    void removeGroupAdmin(std::shared_ptr<TcpConnection> conn, const chat::RemoveGroupAdminReq& req, chat::RemoveGroupAdminRes& res);

    void removeGroupUser(std::shared_ptr<TcpConnection> conn, const chat::RemoveGroupUserReq& req, chat::RemoveGroupUserRes& res);

    void refuseGroup(std::shared_ptr<TcpConnection> conn, const chat::RefuseGroupReq& req, chat::RefuseGroupRes& res);

    void addBlacklist(std::shared_ptr<TcpConnection> conn, const chat::BlacklistAddReq& req, chat::BlacklistAddRes& res);

    void removeBlacklist(std::shared_ptr<TcpConnection> conn, const chat::BlacklistRemoveReq& req, chat::BlacklistRemoveRes& res);

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
    GroupReqModel groupReqModel;
    FileModel fileModel;
    Redis redis;
    BlacklistModel blacklistModel;

    std::unordered_map<std::string, FILE*> fileMap;
    std::unordered_map<std::string, FileInfo> fileInfoMap;
    std::mutex fileMutex;

    std::unordered_map<int, std::shared_ptr<TcpConnection>> userConnMap;
    std::mutex connMutex;

    ChatService() {
        redis.connect();
    }
};