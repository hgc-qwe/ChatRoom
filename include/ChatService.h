#include <unordered_map>
#include "UserModel.h"
#include "FriendModel.h"
#include "OfflineMsg.h"
#include "GroupModel.h"
#pragma once

class ChatService
{
public:
    static ChatService* instance();

    void login(const chat::LoginReq& req, chat::LoginRes& res);
    void reg(const chat::RegisterReq& req, chat::RegisterRes& res);

    void addFriend(const chat::AddFriendReq& req, chat::AddFriendRes& res);

    void oneChat(const chat::OneChatReq& req, chat::OneChatRes& res);

    void createGroup(const chat::CreateGroupReq& req, chat::CreateGroupRes& res);
    void addGroup(const chat::AddGroupReq& req, chat::AddGroupRes& res);

    void groupChat(const chat::GroupChatReq& req, chat::GroupChatRes& res);

    void loginout(const chat::LogoutReq& req, chat::LogoutRes& res);

private:
    UserModel userModel;
    FriendModel friendModel;
    OfflineMsg offlineMsg;
    GroupModel groupModel;
    // std::unordered_map<int, TcpConnection> userConnMap;
    // std::unordered_map<int, MsgHandel> msgHandelMap;
    ChatService() = default;
};