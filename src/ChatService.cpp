#include <iostream>
#include "ChatService.h"
#include "chat.pb.h"

ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

void ChatService::login(const chat::LoginReq& req, chat::LoginRes& res) {
    User user = userModel.query(req.id());
    if (user.getId() == req.id() && user.getPassword() == req.password() && user.getState() == "offline") {
        user.setState("online");
        if (!userModel.updateState(user)) {
            res.set_err(1);
            res.set_errmsg("update state failed");
            return;
        }

        res.set_err(0);
        res.set_errmsg("login success");

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
    } else {
        res.set_err(1);
        res.set_errmsg("login failed");
    }
}

void ChatService::reg(const chat::RegisterReq& req, chat::RegisterRes& res) {
    if (req.name().empty() || req.password().empty()) {
        res.set_err(1);
        res.set_errmsg("register failed");
        return;
    }
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

void ChatService::addFriend(const chat::AddFriendReq& req, chat::AddFriendRes& res) {
    if (friendModel.insert(req.userid(), req.friendid())) {
        res.set_err(0);
        res.set_errmsg("addFriend success");
    } else {
        res.set_err(1);
        res.set_errmsg("addFriend failed");
    }
}

void ChatService::oneChat(const chat::OneChatReq& req, chat::OneChatRes& res) {
    User user = userModel.query(req.toid());
    if (user.getState() == "online") {
        res.set_err(0);
        res.set_errmsg("send success");
        //
    } else {
        offlineMsg.insert(req.toid(), req.msg());
    }
}

void ChatService::createGroup(const chat::CreateGroupReq& req, chat::CreateGroupRes& res) {
    if (req.groupname().empty() || req.groupdesc().empty()) {
        res.set_err(1);
        res.set_errmsg("createGroup failed");
        return;
    } 
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

void ChatService::addGroup(const chat::AddGroupReq& req, chat::AddGroupRes& res) {
    if (groupModel.addGroup(req.userid(), req.groupid(), "normal")) {
        res.set_err(0);
        res.set_errmsg("addGroup success");
    } else {
        res.set_err(1);
        res.set_errmsg("addGroup failed");
    }
}

void ChatService::groupChat(const chat::GroupChatReq& req, chat::GroupChatRes& res) {
    auto users = groupModel.queryGroupUsers(req.userid(), req.groupid());
    for (int id : users) {
        User user = userModel.query(id);
        if (user.getState() == "online") {
            //
        } else {
            offlineMsg.insert(id, req.msg());
        }
    }
}

void ChatService::loginout(const chat::LogoutReq& req, chat::LogoutRes& res) {
    User user = userModel.query(req.userid());
    if (user.getId() == req.userid() && user.getState() == "online") {
        user.setState("offline");
        userModel.updateState(user);

        res.set_err(0);
        res.set_errmsg("Logout success");
    } else {
        res.set_err(1);
        res.set_errmsg("Logout failed");
    }
}