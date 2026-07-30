#include <iostream>
#include "ChatService.h"
#include "Logger.h"
#include "MessageCodec.h"
#include "chat.pb.h"
#include "MessageModel.h"
#include "Message.h"

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
            LOG_ERROR("user {} update state failed", req.id());
        } else {
            res.set_err(0);
            res.set_errmsg("login success");
            LOG_INFO("user {} login success", req.id());
            addUserConn(req.id(), conn);
            conn->setUserId(req.id());

            auto loginUser = res.mutable_user();
            loginUser->set_id(user.getId());
            loginUser->set_name(user.getName());
            loginUser->set_state(user.getState());

            std::vector<Message> offmsgs = messageModel.queryOffline(req.id());

            for (auto& msg : offmsgs) {
                res.add_offlinemsgs(msg.getMsg());
            }
            messageModel.updateStatus(req.id());
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
        LOG_WARN("user {} login failed", req.id());
    }
}

void ChatService::reg(std::shared_ptr<TcpConnection> conn, const chat::RegisterReq& req, chat::RegisterRes& res) {
    if (req.name().empty() || req.password().empty()) {
        res.set_err(1);
        res.set_errmsg("register failed");
        LOG_WARN("register failed name={}", req.name());
    } else {
        User user;
        user.setName(req.name());
        user.setPassword(req.password());
        user.setState("offline");

        if (userModel.insert(user)) {
            res.set_err(0);
            res.set_userid(user.getId());
            res.set_errmsg("register success");
            LOG_INFO("register success userid={}", user.getId());
        } else {
            res.set_err(1);
            res.set_errmsg("register failed");
            LOG_WARN("register failed name={}", req.name());
        }
    }
}

void ChatService::addFriend(std::shared_ptr<TcpConnection> conn, const chat::AddFriendReq& req, chat::AddFriendRes& res) {
    if (req.fromid() == req.toid()) {
        res.set_err(1);
        res.set_errmsg("cannot add yourself");
        return;
    }

    if (friendReqModel.insert(req.fromid(), req.toid())) {
        res.set_err(0);
        res.set_errmsg("add friend request success");
        
        auto target = getUserConn(req.toid());
        if (target) {
            User user = userModel.query(req.fromid());
            chat::FriendRequest notify;
            notify.set_userid(req.fromid());
            notify.set_username(user.getName());

            std::string data;
            notify.SerializeToString(&data);
            target->sendMessage(MessageCodec::encode(chat::FRIEND_NOTIFY_MSG, data));
            LOG_INFO("user {} add friend {} success", req.fromid(), req.toid());
        }
    } else {
        res.set_err(1);
        res.set_errmsg("add friend request failed");
        LOG_WARN("user {} add friend {} failed", req.fromid(), req.toid());
    }
}

void ChatService::queryFriend(std::shared_ptr<TcpConnection> conn, const chat::QueryFriendReq& req, chat::QueryFriendRes& res) {
    std::vector<User> friends = friendModel.query(req.userid());
    for (auto& f : friends) {
        chat::User* item = res.add_friends();
        item->set_id(f.getId());
        item->set_name(f.getName());
        item->set_state(f.getState());
    }
    res.set_err(0);
    res.set_errmsg("query friends success");
    LOG_INFO("user {} query friends", req.userid());
}

void ChatService::acceptFriend(std::shared_ptr<TcpConnection> conn, const chat::AcceptFriendReq& req, chat::AcceptFriendRes& res) {
    if (!friendReqModel.updateStatus(req.friendid(), req.userid(), 1)) {
        res.set_err(1);
        res.set_errmsg("accept failed");
        return;
    }
    friendModel.insert(req.userid(), req.friendid());
    res.set_err(0);
    res.set_errmsg("accept success");
    auto target = getUserConn(req.friendid());
    if (target) {
        User user = userModel.query(req.userid());
        chat::FriendAcceptNotify notify;
        notify.set_userid(req.userid());
        notify.set_username(user.getName());

        std::string data;
        notify.SerializeToString(&data);
        target->sendMessage(MessageCodec::encode(chat::FRIEND_ACCEPT_NOTIFY_MSG, data));
        LOG_INFO("user {} accept friend request success", req.userid());
    }
}

void ChatService::queryFriendreq(std::shared_ptr<TcpConnection> conn, const chat::QueryFriendReqReq& req, chat::QueryFriendReqRes& res) {
    auto requests = friendReqModel.query(req.userid());
    for (auto& r : requests) {
        User user = userModel.query(r.getUserid());
        auto item = res.add_requests();
        item->set_userid(user.getId());
        item->set_username(user.getName());
    }
    res.set_err(0);
    res.set_errmsg("query success");
    LOG_INFO("user {} query friend requests", req.userid());
}

void ChatService::oneChat(std::shared_ptr<TcpConnection> conn, const chat::OneChatReq& req, chat::OneChatRes& res) {
    if (!friendModel.isFriend(req.fromid(), req.toid())) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }
    auto targetConn = getUserConn(req.toid());
    if (targetConn) {
        std::string data;
        req.SerializeToString(&data);
        targetConn->sendMessage(MessageCodec::encode(chat::ONE_CHAT_MSG, data));
        res.set_err(0);
        res.set_errmsg("send success");
        LOG_INFO("{} send message to {}", req.fromid(), req.toid());
    } else {
        if (messageModel.insert(req.fromid(), req.toid(), req.msg())) {
            res.set_err(0);
            res.set_errmsg("message saved");
            LOG_WARN("user {} offline, save message", req.toid());
        } else {
            res.set_err(1);
            res.set_errmsg("send failed");
        }
    }
}

void ChatService::createGroup(std::shared_ptr<TcpConnection> conn, const chat::CreateGroupReq& req, chat::CreateGroupRes& res) {
    if (req.groupname().empty() || req.groupdesc().empty()) {
        res.set_err(1);
        res.set_errmsg("createGroup failed");
        LOG_WARN("user {} create group failed", req.userid());
    } else {
        Group group(req.groupname(), req.groupdesc());
        if (groupModel.createGroup(group)) {
            groupModel.addGroup(req.userid(), group.getId(), "creator");
            res.set_groupid(group.getId());
            res.set_err(0);
            res.set_errmsg("createGroup success");
            LOG_INFO("user {} create group {} success", req.userid(), group.getId());
        } else {
            res.set_err(1);
            res.set_errmsg("createGroup failed");
            LOG_WARN("user {} create group failed", req.userid());
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
            LOG_INFO("user {} send group message {}", req.userid(), req.groupid());
        } else {
            messageModel.insert(req.userid(), id, req.msg());
            LOG_WARN("group memeber {} offline, save message", id);
        }
    }
    res.set_err(0);
    res.set_errmsg("groupChat success");
    LOG_INFO("user {} send group message {}", req.userid(), req.groupid());
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
        if (userModel.updateState(user)) {
            LOG_INFO("user {} disconnected", userid);
        } else {
            LOG_ERROR("user {} update offline failed", userid);
        }
    }
}