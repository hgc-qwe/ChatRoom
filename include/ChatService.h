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

    void login(const int id, const std::string& pwd);
    void reg(const std::string& name, const std::string pwd);

    void addFriend(const int userid, const int friendid);

    void oneChat(const int fromid, const int toid, const std::string& msg);

    void createGroup(const int userid, const std::string& name, const std::string& desc);
    void addGroup(int userid, int groupid, std::string role);

    void groupChat(const int userid, const int groupid, const std::string& msg);

    void loginout(const int id);

private:
    UserModel userModel;
    FriendModel friendModel;
    OfflineMsg offlineMsg;
    GroupModel groupModel;
    // std::unordered_map<int, TcpConnection> userConnMap;
    // std::unordered_map<int, MsgHandel> msgHandelMap;
    ChatService() = default;
};