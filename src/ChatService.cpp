#include <iostream>
#include "ChatService.h"
#include "MessageCodec.h"
#include "chat.pb.h"

ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

void ChatService::login(std::shared_ptr<TcpConnection> conn, const chat::LoginReq& req, chat::LoginRes& res) {
    User user = userModel.query(req.id());
    if (user.getId() == req.id() && user.getPassword() == req.password() && user.getState() == "offline") {
        user.setState("online");
        if (!userModel.updateState(user)) {
            res.set_err(1);
            res.set_errmsg("update state failed");
        } else {
            res.set_err(0);
            res.set_errmsg("login success");
            addUserConn(req.id(), conn);
            conn->setUserId(req.id());

            auto loginUser = res.mutable_user();
            loginUser->set_id(user.getId());
            loginUser->set_name(user.getName());
            loginUser->set_state(user.getState());

            std::vector<std::string> offmsgs = offlineMsg.query(req.id());

            for (auto& msg : offmsgs) {
                res.add_offlinemsgs(msg);
            }
            offlineMsg.remove(req.id());
            std::vector<User> users = friendModel.query(req.id());

            for (auto& u : users) {
                chat::User* user = res.add_friends();
                user->set_id(u.getId());
                user->set_name(u.getName());
                user->set_state(u.getState());
            }
            std::vector<Group> groups = groupModel.queryGroups(req.id());

            for (auto& g : groups) {
                chat::Group* group = res.add_groups();
                group->set_id(g.getId());
                group->set_name(g.getName());
                group->set_desc(g.getDesc());
            }
        }
    } else {
        res.set_err(1);
        res.set_errmsg("login failed");
    }
}

void ChatService::reg(std::shared_ptr<TcpConnection> conn, const chat::RegisterReq& req, chat::RegisterRes& res) {
    if (req.name().empty() || req.password().empty()) {
        res.set_err(1);
        res.set_errmsg("register failed");
    } else {
        User user;
        user.setName(req.name());
        user.setPassword(req.password());
        user.setState("offline");

        if (userModel.insert(user)) {
            res.set_err(0);
            res.set_userid(user.getId());
            res.set_errmsg("register success");
        } else {
            res.set_err(1);
            res.set_errmsg("register failed");
        }
    }
}

void ChatService::addFriend(std::shared_ptr<TcpConnection> conn, const chat::AddFriendReq& req, chat::AddFriendRes& res) {
    if (friendModel.insert(req.userid(), req.friendid())) {
        res.set_err(0);
        res.set_errmsg("addFriend success");
    } else {
        res.set_err(1);
        res.set_errmsg("addFriend failed");
    }
}

void ChatService::oneChat(std::shared_ptr<TcpConnection> conn, const chat::OneChatReq& req, chat::OneChatRes& res) {
    auto targetConn = getUserConn(req.toid());
    if (targetConn) {
        std::string data;
        req.SerializeToString(&data);
        targetConn->sendMessage(MessageCodec::encode(chat::ONE_CHAT_MSG, data));
        res.set_err(0);
        res.set_errmsg("send success");
    } else {
        offlineMsg.insert(req.toid(), req.fromid(), req.msg());
        res.set_err(1);
        res.set_errmsg("user offline");
    }
}

void ChatService::createGroup(std::shared_ptr<TcpConnection> conn, const chat::CreateGroupReq& req, chat::CreateGroupRes& res) {
    if (req.groupname().empty() || req.groupdesc().empty()) {
        res.set_err(1);
        res.set_errmsg("createGroup failed");
    } else {
        Group group(req.groupname(), req.groupdesc());
        if (groupModel.createGroup(group)) {
            groupModel.addGroup(req.userid(), group.getId(), "creator");
            res.set_groupid(group.getId());
            res.set_err(0);
            res.set_errmsg("createGroup success");
        } else {
            res.set_err(1);
            res.set_errmsg("createGroup failed");
        }
    }
}

void ChatService::addGroup(std::shared_ptr<TcpConnection> conn, const chat::AddGroupReq& req, chat::AddGroupRes& res) {
    if (groupModel.addGroup(req.userid(), req.groupid(), "normal")) {
        res.set_err(0);
        res.set_errmsg("addGroup success");
    } else {
        res.set_err(1);
        res.set_errmsg("addGroup failed");
    }
}

void ChatService::groupChat(std::shared_ptr<TcpConnection> conn, const chat::GroupChatReq& req, chat::GroupChatRes& res) {
    auto users = groupModel.queryGroupUsers(req.userid(), req.groupid());
    for (int id : users) {
        if (id == req.userid()) continue;
        auto userconn = getUserConn(id);
        if (userconn) {
            std::string data;
            req.SerializeToString(&data);
            userconn->sendMessage(MessageCodec::encode(chat::GROUP_CHAT_MSG, data));
        } else {
            offlineMsg.insert(id, req.userid(), req.msg());
        }
    }
    res.set_err(0);
    res.set_errmsg("groupChat success");
}

void ChatService::logout(std::shared_ptr<TcpConnection> conn, const chat::LogoutReq& req, chat::LogoutRes& res) {
    User user = userModel.query(req.userid());
    if (user.getId() == req.userid() && user.getState() == "online") {
        clientClose(req.userid());
        res.set_err(0);
        res.set_errmsg("logout success");
    } else {
        res.set_err(1);
        res.set_errmsg("logout failed");
    }
}

void ChatService::addUserConn(int userid, std::shared_ptr<TcpConnection> conn) {
    std::lock_guard<std::mutex> lock(connMutex);
    userConnMap[userid] = conn;
}

std::shared_ptr<TcpConnection> ChatService::getUserConn(int userid) {
    std::lock_guard<std::mutex> lock(connMutex);
    auto it = userConnMap.find(userid);
    if (it != userConnMap.end()) return it->second;
    return nullptr;
}

void ChatService::removeUser(int userid) {
    std::lock_guard<std::mutex> lock(connMutex);
    auto it = userConnMap.find(userid);
    if (it != userConnMap.end()) userConnMap.erase(it);
}

void ChatService::clientClose(int userid) {
    removeUser(userid);
    User user = userModel.query(userid);
    if (user.getId() == userid && user.getState() == "online") {
        user.setState("offline");
        userModel.updateState(user);
    }
}