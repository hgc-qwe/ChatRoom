#include <iostream>
#include "ChatService.h"
#include "Logger.h"
#include "MessageCodec.h"
#include "chat.pb.h"
#include "MessageModel.h"
#include "Message.h"
#include "GroupMessageModel.h"
#include "Util.h"

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
            redis.set("user:state:" + std::to_string(req.id()), "online");
            conn->setUserId(req.id());

            auto loginUser = res.mutable_user();
            loginUser->set_id(user.getId());
            loginUser->set_name(user.getName());
            loginUser->set_state(user.getState());

            std::string key = "offline:msg:" + std::to_string(req.id());
            std::vector<std::string> redisMsgs;
            redis.getList(key, redisMsgs);

            for (auto& data : redisMsgs) {
                chat::OfflineMsg offline;
                if (!offline.ParseFromString(data)) continue;
                auto item = res.add_offlinemsgs();
                item->set_fromid(offline.fromid());
                item->set_toid(offline.toid());
                item->set_msg(offline.msg());
                item->set_time(offline.time());
            }
            if (!redisMsgs.empty()) redis.del(key);

            std::string groupkey = "offline:group:" + std::to_string(req.id());
            std::vector<std::string> groupMsg;
            redis.getList(groupkey, groupMsg);

            for (auto& data : groupMsg) {
                chat::OfflineGroupMsg msg;
                if (!msg.ParseFromString(data)) continue;
                auto item = res.add_offlinegroupmsg();
                item->set_groupid(msg.groupid());
                item->set_userid(msg.userid());
                item->set_msg(msg.msg());
                item->set_time(msg.time());
            }
            if (!groupMsg.empty()) redis.del(groupkey);

            messageModel.updateStatus(req.id());
            
            std::vector<User> users = friendModel.query(req.id());
            for (auto& u : users) {
                chat::User* user = res.add_friends();
                user->set_id(u.getId());
                user->set_name(u.getName());
                std::string state;
                if(redis.get("user:state:" + std::to_string(u.getId()), state)) {
                    user->set_state(state);
                } else {
                    user->set_state("offline");
                }
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
        
        std::string state;
        if(redis.get("user:state:" + std::to_string(f.getId()), state)) {
            item->set_state(state);
        } else {
            item->set_state("offline");
        }
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

void ChatService::deleteFriend(std::shared_ptr<TcpConnection> conn, const chat::DeleteFriendReq& req, chat::DeleteFriendRes& res) {
    bool ret1 = friendModel.remove(req.userid(), req.friendid());
    bool ret2 = friendModel.remove(req.friendid(), req.userid());
    if (ret1 && ret2) {
        res.set_err(0);
        res.set_errmsg("delete friend success");
        LOG_INFO("user {} delete friend requests", req.userid());
    } else {
        res.set_err(1);
        res.set_errmsg("delete friend failed");
    }
}

void ChatService::queryHistoryMsg(std::shared_ptr<TcpConnection> conn, const chat::HistoryMsgReq& req, chat::HistoryMsgRes& res) {
    std::vector<Message> message  = messageModel.queryHistory(req.fromid(), req.toid());
    for (auto& msg : message) {
        auto item = res.add_msgs();
        item->set_fromid(msg.getFromid());
        item->set_toid(msg.getToid());
        item->set_msg(msg.getMsg());
        item->set_time(msg.getTime());
    }
    res.set_err(0);
    res.set_errmsg("query history success");
    LOG_INFO("query user {} and user {} history message", req.fromid(), req.toid());
}

void ChatService::oneChat(std::shared_ptr<TcpConnection> conn, const chat::OneChatReq& req, chat::OneChatRes& res) {
    if (!friendModel.isFriend(req.fromid(), req.toid())) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }
    auto targetConn = getUserConn(req.toid());
     std::string data;
     req.SerializeToString(&data);
    if (targetConn) {
        targetConn->sendMessage(MessageCodec::encode(chat::ONE_CHAT_MSG, data));
        messageModel.insert(req.fromid(), req.toid(), req.msg());
        res.set_err(0);
        res.set_errmsg("send success");
        LOG_INFO("{} send message to {}", req.fromid(), req.toid());
    } else {
        chat::OfflineMsg offline;
        offline.set_fromid(req.fromid());
        offline.set_toid(req.toid());
        offline.set_msg(req.msg());
        std::string now = getCurrentTime();
        offline.set_time(now);
        
        std::string value;
        offline.SerializeToString(&value);

        std::string key = "offline:msg:" + std::to_string(req.toid());
        redis.pushList(key, value);

        messageModel.insert(req.fromid(), req.toid(), req.msg());

        res.set_err(0);
        res.set_errmsg("offline save");
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
    if(!groupMessageModel.insert(req.groupid(), req.userid(), req.msg())) {
        res.set_err(1);
        res.set_errmsg("save group message failed");
        return;
    }
    auto users = groupModel.queryGroupUsers(req.userid(), req.groupid());
    std::string data;
    req.SerializeToString(&data);
    for (auto id : users) {
        if (id == req.userid()) continue;
        auto userconn = getUserConn(id);
        if (userconn) {
            userconn->sendMessage(MessageCodec::encode(chat::GROUP_CHAT_MSG, data));
        } else {
            chat::OfflineGroupMsg offline;
            offline.set_groupid(req.groupid());
            offline.set_userid(req.userid());
            offline.set_msg(req.msg());
            std::string now = getCurrentTime();
            offline.set_time(now);
            
            std::string value;
            offline.SerializeToString(&value);

            redis.pushList("offline:group:" + std::to_string(id),value);
        }
    }
    res.set_err(0);
    res.set_errmsg("groupChat success");
    LOG_INFO("user {} send group message {}", req.userid(), req.groupid());
}

void ChatService::queryGroupHistoryMsg(std::shared_ptr<TcpConnection> conn, const chat::GroupHistoryMsgReq& req, chat::GroupHistoryMsgRes& res) {
    std::vector<GroupMessage> message  = groupMessageModel.query(req.groupid());
    for (auto& msg : message) {
        auto item = res.add_msgs();
        item->set_groupid(msg.getGroupid());
        item->set_userid(msg.getUserid());
        item->set_msg(msg.getMsg());
        item->set_time(msg.getTime());
    }
    res.set_err(0);
    res.set_errmsg("query group history success");
    LOG_INFO("query group {} history message", req.groupid());
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
            redis.set("user:state:" + std::to_string(userid), "offline");
            LOG_INFO("user {} disconnected", userid);
        } else {
            LOG_ERROR("user {} update offline failed", userid);
        }
    }
}