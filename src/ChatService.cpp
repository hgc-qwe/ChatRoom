#include <iostream>
#include "ChatService.h"

ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

void ChatService::login(const int id, const std::string& pwd) {
    User user = userModel.query(id);
    if (user.getId() == id && user.getPassword() == pwd && user.getState() == "offline") {
        user.setState("online");
        userModel.updateState(user);

        std::vector<std::string> offmsgs = offlineMsg.query(id);
        offlineMsg.remove(id);
        std::vector<User> users = friendModel.query(id);
        std::vector<Group> groups = groupModel.queryGroups(id);

        std::cout << "login success" << std::endl;
    } else {
        std::cout << "login failed" << std::endl;
    }
}

void ChatService::reg(const std::string& name, const std::string pwd) {
    User user;
    if (name != "" && pwd != "") {
        user.setName(name);
        user.setPassword(pwd);
        user.setState("offline");
        userModel.insert(user);
        std::cout << "register success" << std::endl;
    } else {
        std::cout << "register failed" << std::endl;
    }
}

void ChatService::addFriend(const int userid, const int friendid) {
    if (friendModel.insert(userid, friendid)) {
        std::cout << "addFriend success" << std::endl;
    } else {
        std::cout << "addFriend failed" << std::endl;
    }
}

void ChatService::oneChat(const int fromid, const int toid, const std::string& msg) {
    User user = userModel.query(toid);
    if (user.getState() == "online") {
//
    } else {
        offlineMsg.insert(toid, msg);
    }
}

void ChatService::createGroup(const int userid, const std::string& name, const std::string& desc) {
    if (name != "" && desc != "") {  
        Group group(name, desc);
        if (groupModel.createGroup(group)) {
            groupModel.addGroup(userid, group.getId(), "craetor");
        }
    }
}

void ChatService::addGroup(int userid, int groupid, std::string role) {
    groupModel.addGroup(userid, groupid, role);
}

void ChatService::groupChat(const int userid, const int groupid, const std::string& msg) {
    auto users = groupModel.queryGroupUsers(userid, groupid);
    for (int id : users) {
        User user = userModel.query(id);
        if (user.getState() == "online") {
            //
        } else {
            offlineMsg.insert(id, msg);
        }
    }
}

void ChatService::loginout(const int id) {
    User user = userModel.query(id);
    if (user.getId() == id && user.getState() == "online") {
        user.setState("offline");
        userModel.updateState(user);

        std::cout << "loginout success" << std::endl;
    } else {
        std::cout << "loginout failed" << std::endl;
    }
}