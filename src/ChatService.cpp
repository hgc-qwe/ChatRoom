#include <iostream>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include "ChatService.h"
#include "Logger.h"
#include "MessageCodec.h"
#include "chat.pb.h"
#include "MessageModel.h"
#include "Message.h"
#include "GroupMessageModel.h"
#include "Util.h"
#include "TcpConnection.h"
#include "FileModel.h"
#include "BlacklistModel.h"
#include "VerifyCode.h"
#include "EmailSender.h"

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
            addUserConn(conn, req.id());
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

            std::string filekey = "offline:file:" + std::to_string(req.id());
            std::vector<std::string> files;
            redis.getList(filekey, files);

            for (auto& data : files) {
                chat::OfflineFile file;
                if (!file.ParseFromString(data)) continue;
                auto item = res.add_offlinefiles();
                item->set_fromid(file.fromid());
                item->set_toid(file.toid());
                item->set_filename(file.filename());
                item->set_filesize(file.filesize());
                item->set_fileid(file.fileid());
            }
            if (!files.empty()) redis.del(filekey);
            else {
                auto offFiles = fileModel.queryOffline(req.id());
                for (auto& file : offFiles) {
                    auto item = res.add_offlinefiles();
                    item->set_fromid(file.getFromid());
                    item->set_toid(file.getToid());
                    item->set_filename(file.getFilename());
                    item->set_filesize(file.getFilesize());
                    item->set_fileid(file.getFileid());
                }
            }

            std::string groupFileKey = "offline:groupfile:" + std::to_string(req.id());
            std::vector<std::string> groupFiles;

            redis.getList(groupFileKey, groupFiles);
            for (auto& data : groupFiles) {
                chat::GroupFileNotify file;
                if (!file.ParseFromString(data)) continue;

                auto item = res.add_offlinegroupfiles();
                item->set_fromid(file.fromid());
                item->set_fromname(file.fromname());
                item->set_groupid(file.groupid());
                item->set_groupname(file.groupname());
                item->set_fileid(file.fileid());
                item->set_filename(file.filename());
                item->set_filesize(file.filesize());
            }
            if (!groupFiles.empty()) redis.del(groupFileKey);

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

            std::vector<FriendRequest> friendRequest = friendReqModel.query(req.id());
            for (auto& request : friendRequest) {
                User fromuser = userModel.query(request.getFromid());
                auto item = res.add_offlinefriendrequests();
                item->set_userid(request.getFromid());
                item->set_username(fromuser.getName());
            }

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
        LOG_WARN("register failed name = {}", req.name());
        return;
    }

    std::string verifiedKey;
    if (req.email().empty()) {
        res.set_err(1);
        res.set_errmsg("email empty");
        return;
    }
    verifiedKey = "verified:email:" + req.email() + ":1";

    Redis redis;
    if (!redis.connect()) {
        res.set_err(1);
        res.set_errmsg("redis error");
        return;
    }

    std::string verified;
    if (!redis.get(verifiedKey, verified)) {
        res.set_err(1);
        res.set_errmsg("please verify code first");
        return;
    }

    User exist = userModel.queryByEmail(req.email());
    if (exist.getId() != -1) {
        res.set_err(1);
        res.set_errmsg("account exists");
        return;
    }

    User user;
    user.setName(req.name());
    user.setPassword(req.password());
    user.setState("offline");
    user.setEmail(req.email());

    if (userModel.insert(user)) {
        redis.del(verifiedKey);
        res.set_err(0);
        res.set_userid(user.getId());
        res.set_errmsg("register success");
        LOG_INFO("register success userid = {}", user.getId());
    } else {
        res.set_err(1);
        res.set_errmsg("register failed");
        LOG_WARN("register failed name = {}", req.name());
    }
}

void ChatService::addFriend(std::shared_ptr<TcpConnection> conn, const chat::AddFriendReq& req, chat::AddFriendRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int fromid = conn->getUserId();
    int toid = req.toid();

    if (fromid == toid) {
        res.set_err(1);
        res.set_errmsg("cannot add yourself");
        return;
    }
    if (!userModel.isExist(toid)) {
        res.set_err(1);
        res.set_errmsg("toid not exist");
        return;
    }
    if (friendModel.isFriend(fromid, toid)) {
        res.set_err(1);
        res.set_errmsg("you are friends");
        return;
    }

    if (friendReqModel.insert(fromid, toid)) {
        res.set_err(0);
        res.set_errmsg("add friend request success");
        
        auto target = getUserConn(toid);
        if (target) {
            User user = userModel.query(fromid);
            chat::FriendRequest notify;
            notify.set_userid(fromid);
            notify.set_username(user.getName());

            std::string data;
            notify.SerializeToString(&data);
            target->sendMessage(MessageCodec::encode(chat::FRIEND_NOTIFY_MSG, data));
            LOG_INFO("user {} add friend {} success", fromid, toid);
        }
    } else {
        res.set_err(1);
        res.set_errmsg("add friend request failed");
        LOG_WARN("user {} add friend {} failed", fromid, toid);
    }
}

void ChatService::queryFriend(std::shared_ptr<TcpConnection> conn, const chat::QueryFriendReq& req, chat::QueryFriendRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();

    std::vector<User> friends = friendModel.query(userid);
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
    LOG_INFO("user {} query friends", userid);
}

void ChatService::acceptFriend(std::shared_ptr<TcpConnection> conn, const chat::AcceptFriendReq& req, chat::AcceptFriendRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    int friendid = req.friendid();
    if (!userModel.isExist(friendid)) {
        res.set_err(1);
        res.set_errmsg("friendid not exist");
        return;
    }
    if (friendModel.isFriend(userid, friendid)) {
        res.set_err(1);
        res.set_errmsg("you are friends");
        return;
    }
    if (!friendReqModel.isApplied(friendid, userid)) {
        res.set_err(1);
        res.set_errmsg("application not exist");
        return;
    }

    if (!friendReqModel.updateStatus(friendid, userid, 1)) {
        res.set_err(1);
        res.set_errmsg("accept failed");
        return;
    }
    if (!friendModel.insert(userid, friendid)) {
        res.set_err(1);
        res.set_errmsg("accpet failed");
        return;
    }
    res.set_err(0);
    res.set_errmsg("accept success");
    auto target = getUserConn(friendid);
    if (target) {
        User user = userModel.query(userid);
        chat::FriendAcceptNotify notify;
        notify.set_userid(userid);
        notify.set_username(user.getName());

        std::string data;
        notify.SerializeToString(&data);
        target->sendMessage(MessageCodec::encode(chat::FRIEND_ACCEPT_NOTIFY_MSG, data));
        LOG_INFO("user {} accept friend request success", userid);
    }
}

void ChatService::queryFriendreq(std::shared_ptr<TcpConnection> conn, const chat::QueryFriendReqReq& req, chat::QueryFriendReqRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();

    auto requests = friendReqModel.query(userid);
    for (auto& r : requests) {
        User user = userModel.query(r.getFromid());
        auto item = res.add_requests();
        item->set_userid(r.getFromid());
        item->set_username(user.getName());
    }
    res.set_err(0);
    res.set_errmsg("query success");
    LOG_INFO("user {} query friend requests", userid);
}

void ChatService::deleteFriend(std::shared_ptr<TcpConnection> conn, const chat::DeleteFriendReq& req, chat::DeleteFriendRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    int friendid = req.friendid();
    if (!userModel.isExist(friendid)) {
        res.set_err(1);
        res.set_errmsg("friendid not exist");
        return;
    }
    if (!friendModel.isFriend(userid, friendid)) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }

    bool ret1 = friendModel.remove(userid, friendid);
    bool ret2 = friendModel.remove(friendid, userid);
    if (ret1 && ret2) {
        res.set_err(0);
        res.set_errmsg("delete friend success");
        LOG_INFO("user {} delete friend requests", userid);
    } else {
        res.set_err(1);
        res.set_errmsg("delete friend failed");
    }
}

void ChatService::queryHistoryMsg(std::shared_ptr<TcpConnection> conn, const chat::HistoryMsgReq& req, chat::HistoryMsgRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    int friendid = req.friendid();
    if (!userModel.isExist(friendid)) {
        res.set_err(1);
        res.set_errmsg("friendid not exist");
        return;
    }
    if (!friendModel.isFriend(userid, friendid)) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }
    
    std::vector<Message> message  = messageModel.queryHistory(userid, friendid);
    if (message.empty()) {
        res.set_err(1);
        res.set_errmsg("no message");
        return;
    }
    for (auto& msg : message) {
        auto item = res.add_msgs();
        item->set_fromid(msg.getFromid());
        item->set_toid(msg.getToid());
        item->set_msg(msg.getMsg());
        item->set_time(msg.getTime());
    }
    std::vector<File> file = fileModel.queryFriendFile(userid, friendid);
    if (file.empty()) {
        res.set_err(1);
        res.set_errmsg("no file");
        return;
    }
    for (auto& f : file) {
        auto item = res.add_files();
        item->set_fileid(f.getFileid());
        item->set_filename(f.getFilename());
        item->set_filesize(f.getFilesize());
        item->set_fromid(f.getFromid());
        item->set_toid(f.getToid());
    }
    res.set_err(0);
    res.set_errmsg("query history success");
    LOG_INFO("query user {} and user {} history message", userid, friendid);
}

void ChatService::oneChat(std::shared_ptr<TcpConnection> conn, const chat::OneChatReq& req, chat::OneChatRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int fromid = conn->getUserId();
    int toid = req.toid();
    User user = userModel.query(fromid);
    
    if (fromid == toid) {
        res.set_err(1);
        res.set_errmsg("cannot chat with yourself");
        return;
    }
    if (!userModel.isExist(toid)) {
        res.set_err(1);
        res.set_errmsg("friendid not exist");
        return;
    }
    if (!friendModel.isFriend(fromid, toid)) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }
    if (blacklistModel.isBlacked(fromid, toid) || blacklistModel.isBlacked(toid, fromid)) {
        res.set_err(1);
        res.set_errmsg("message blacked");
        return;
    }
    auto targetConn = getUserConn(req.toid());
    if (targetConn) {
        chat::OneChatNotify notify;
        notify.set_fromid(fromid);
        notify.set_fromname(user.getName());
        notify.set_msg(req.msg());
        notify.set_time(getCurrentTime());

        std::string data;
        notify.SerializeToString(&data);

        targetConn->sendMessage(MessageCodec::encode(chat::ONE_CHAT_MSG, data));
        
        messageModel.insert(fromid, toid, req.msg(), user.getName());
        
        res.set_err(0);
        res.set_errmsg("send success");
        LOG_INFO("{} send message to {}", fromid, toid);
    } else {
        chat::OfflineMsg offline;
        offline.set_fromid(fromid);
        offline.set_toid(toid);
        offline.set_msg(req.msg());
        std::string now = getCurrentTime();
        offline.set_time(now);
        
        std::string value;
        offline.SerializeToString(&value);

        std::string key = "offline:msg:" + std::to_string(toid);
        redis.pushList(key, value);

        messageModel.insert(fromid, toid, req.msg(), user.getName());

        res.set_err(0);
        res.set_errmsg("offline save");
    }
}

void ChatService::createGroup(std::shared_ptr<TcpConnection> conn, const chat::CreateGroupReq& req, chat::CreateGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    int friendid = req.friendid();
    if (userid == friendid) {
        res.set_err(1);
        res.set_errmsg("cannot invite yourself");
        return;
    }
    if (!userModel.isExist(friendid)) {
        res.set_err(1);
        res.set_errmsg("friendid not exist");
        return;
    }
    if (!friendModel.isFriend(userid, friendid)) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }
    if (blacklistModel.isBlacked(userid, friendid) || blacklistModel.isBlacked(friendid, userid)) {
        res.set_err(1);
        res.set_errmsg("message blacked");
        return;
    }
    
    if (req.groupname().empty() || req.groupdesc().empty()) {
        res.set_err(1);
        res.set_errmsg("createGroup failed");
        LOG_WARN("user {} create group failed", userid);
    } else {
        Group group(req.groupname(), req.groupdesc());
        if (groupModel.createGroup(group)) {
            if(groupModel.addGroup(userid, group.getId(), "owner") && groupModel.addGroup(friendid, group.getId(), "normal")) {
                res.set_groupid(group.getId());
                res.set_err(0);
                res.set_errmsg("createGroup success");
                LOG_INFO("user {} create group {} success", userid, group.getId());
                auto target = getUserConn(friendid);
                if (target) {
                    User user = userModel.query(userid);
                    chat::InviteNotify notify;
                    notify.set_ownerid(userid);
                    notify.set_groupname(group.getName());
                    notify.set_groupid(group.getId());

                    std::string data;
                    notify.SerializeToString(&data);
                    target->sendMessage(MessageCodec::encode(chat::INVITE_NOTIFY_MSG, data));
                    LOG_INFO("owner {} invite friend {} success", userid, friendid);
                }
            } else {
                res.set_err(1);
                res.set_errmsg("createGroup failed");
                LOG_WARN("user {} create group failed", userid);
            }
        } else {
            res.set_err(1);
            res.set_errmsg("createGroup failed");
            LOG_WARN("user {} create group failed", userid);
        }
    }
}

void ChatService::groupChat(std::shared_ptr<TcpConnection> conn, const chat::GroupChatReq& req, chat::GroupChatRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    User user = userModel.query(userid);
    
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    } 
    if(!groupMessageModel.insert(req.groupid(), userid, req.msg(), user.getName())) {
        res.set_err(1);
        res.set_errmsg("save group message failed");
        return;
    }
    auto users = groupModel.queryGroupUsers(userid, req.groupid());
    for (auto id : users) {
        if (id == userid) continue;
        auto userconn = getUserConn(id);
        if (userconn) {
            chat::GroupChatNotify notify;
            notify.set_groupid(req.groupid());
            notify.set_msg(req.msg());
            notify.set_time(getCurrentTime());
            notify.set_userid(userid);
            notify.set_username(user.getName());

            std::string data;
            notify.SerializeToString(&data);

            userconn->sendMessage(MessageCodec::encode(chat::GROUP_CHAT_MSG, data));
        } else {
            chat::OfflineGroupMsg offline;
            offline.set_groupid(req.groupid());
            offline.set_userid(userid);
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
    LOG_INFO("user {} send group message {}", userid, req.groupid());
}

void ChatService::queryGroupHistoryMsg(std::shared_ptr<TcpConnection> conn, const chat::GroupHistoryMsgReq& req, chat::GroupHistoryMsgRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    } 

    std::vector<GroupMessage> message  = groupMessageModel.query(req.groupid());
    for (auto& msg : message) {
        auto item = res.add_msgs();
        item->set_groupid(msg.getGroupid());
        item->set_userid(msg.getUserid());
        item->set_msg(msg.getMsg());
        item->set_time(msg.getTime());
    }
    std::vector<File> file = fileModel.queryGroupFile(req.groupid());
    if (file.empty()) {
        res.set_err(1);
        res.set_errmsg("no file");
        return;
    }
    for (auto& f : file) {
        auto item = res.add_files();
        item->set_groupid(f.getGroupid());
        item->set_fileid(f.getFileid());
        item->set_filename(f.getFilename());
        item->set_filesize(f.getFilesize());
        item->set_userid(f.getFromid());
    }
    res.set_err(0);
    res.set_errmsg("query group history success");
    LOG_INFO("query group {} history message", req.groupid());
}

void ChatService::fileStart(std::shared_ptr<TcpConnection> conn, const chat::FileStartReq& req, chat::FileStartRes& res) {
    int fromid = conn->getUserId();
    
    mkdir("./files", 0755);
    std::string path = "./files/" + req.fileid();
    FILE* fp = fopen(path.c_str(), "wb");
    if (fp == nullptr) {
        res.set_err(1);
        res.set_errmsg("open file failed");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(fileMutex);
        fileMap[req.fileid()] = fp;
        FileInfo info;
        info.fromid = fromid;
        info.toid = req.toid();
        info.groupid = req.groupid();
        info.filename = req.filename();
        info.filesize = req.filesize();
        info.fileid = req.fileid();
        info.path = path;
        fileInfoMap[req.fileid()] = info;
    }
    conn->setUploading(true);
    LOG_INFO("start receive file {} size {}", req.filename(), req.filesize());
    res.set_err(0);
    res.set_errmsg("start success");
}

void ChatService::fileChunk(std::shared_ptr<TcpConnection> conn, const chat::FileChunkReq& req, chat::FileChunkRes& res) {
    std::lock_guard<std::mutex> lock(fileMutex);
    auto it = fileMap.find(req.fileid());
    if (it == fileMap.end()) {
        res.set_err(1);
        res.set_errmsg("file not found");
        return;
    }
    fwrite(req.data().data(), 1, req.data().size(), it->second);
    res.set_err(0);
    res.set_errmsg("chunk success");
}

void ChatService::fileEnd(std::shared_ptr<TcpConnection> conn, const chat::FileEndReq& req, chat::FileEndRes& res) {
    FileInfo info;
    {
        std::lock_guard<std::mutex> lock(fileMutex);
        auto it = fileMap.find(req.fileid());
        if (it == fileMap.end()) {
            res.set_err(1);
            res.set_errmsg("file not found");
            return;
        }
        fclose(it->second);
        fileMap.erase(it);
        info = fileInfoMap[req.fileid()];
        fileInfoMap.erase(req.fileid());

        conn->setUploading(false);
    }
    LOG_INFO("receive file {} finish", req.fileid());

    if (info.groupid == 0) {
        File file;
        User user = userModel.query(info.fromid);
        file.setFromname(user.getName());
        file.setFromid(info.fromid);
        file.setToid(info.toid);
        file.setFilename(info.filename);
        file.setFilesize(info.filesize);
        file.setFileid(info.fileid);
        file.setStatus(0);
        file.setType(0);
        file.setGroupid(0);
        if(!fileModel.insert(file)) {
            LOG_ERROR("save file {} failed", req.fileid());
            res.set_err(1);
            res.set_errmsg("save file info failed");
            return;
        }
        auto target = getUserConn(info.toid);
        if (!target) {
            chat::OfflineFile offline;
            offline.set_fromid(info.fromid);
            offline.set_toid(info.toid);
            offline.set_filename(info.filename);
            offline.set_filesize(info.filesize);
            offline.set_fileid(info.fileid);

            std::string data;
            offline.SerializeToString(&data);
            redis.pushList("offline:file:" + std::to_string(info.toid), data);
        }
    } else {
        Group group = groupModel.query(info.groupid);
        User user = userModel.query(info.fromid);
        int groupid = info.groupid;

        File file;
        file.setFromname(user.getName());
        file.setFromid(info.fromid);
        file.setToid(0);
        file.setFilename(info.filename);
        file.setFilesize(info.filesize);
        file.setFileid(info.fileid);
        file.setStatus(0);
        file.setType(1);
        file.setGroupid(info.groupid);

        if (!fileModel.insert(file)) LOG_INFO("save group file {} failed", info.fileid);

        auto members = groupModel.queryGroupUsers(info.fromid, info.groupid);
        for (int memberid : members) {
            auto target = getUserConn(memberid);
            if (target) {
                chat::GroupFileNotify notify;
                notify.set_fromid(info.fromid);
                notify.set_fromname(user.getName());
                notify.set_groupid(info.groupid);
                notify.set_groupname(group.getName());
                notify.set_fileid(info.fileid);
                notify.set_filename(info.filename);
                notify.set_filesize(info.filesize);

                std::string data;
                notify.SerializeToString(&data);

                target->sendMessage(MessageCodec::encode(chat::GROUP_FILE_NOTIFY_MSG, data));
            } else {
                chat::GroupFileNotify notify;
                notify.set_fromid(info.fromid);
                notify.set_fromname(user.getName());
                notify.set_groupid(info.groupid);
                notify.set_groupname(group.getName());
                notify.set_fileid(info.fileid);
                notify.set_filename(info.filename);
                notify.set_filesize(info.filesize);

                std::string data;
                notify.SerializeToString(&data);
                
                redis.pushList("offline:groupfile:" + std::to_string(memberid), data);
            }
        }
    }

    res.set_err(0);
    res.set_errmsg("upload success");
}

void ChatService::sendFile(std::shared_ptr<TcpConnection> conn, const FileInfo& info) {
    conn->startFileDelivery(info.fileid, info.filename, info.path, info.filesize, info.toid);
}

void ChatService::downloadFile(std::shared_ptr<TcpConnection> conn, const chat::DownloadFileReq& req, chat::DownloadFileRes& res) {
    File file = fileModel.queryByFileid(req.fileid());
    if (file.getFileid().empty()) {
        res.set_err(1);
        res.set_errmsg("file not exist");
        return;
    }
    
    std::string path = "./files/" + req.fileid();
    if (!std::filesystem::exists(path)) {
        res.set_err(1);
        res.set_errmsg("file not found on server");
        return;
    }

    uint64_t filesize = std::filesystem::file_size(path);
    bool ok = conn->startDownload(file.getFileid(), file.getFilename(), path, filesize);

    if (!ok) {
        res.set_err(1);
        res.set_errmsg("start download failed");
        return;
    }
    conn->setDownloading(true);
    fileModel.updateStatus(req.fileid());
    res.set_err(0);
    res.set_errmsg("download success");
}

void ChatService::logout(std::shared_ptr<TcpConnection> conn, const chat::LogoutReq& req, chat::LogoutRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    User user = userModel.query(userid);
    if (user.getId() == userid && user.getState() == "online") {
        clientClose(userid);
        res.set_err(0);
        res.set_errmsg("logout success");
    } else {
        res.set_err(1);
        res.set_errmsg("logout failed");
    }
}

void ChatService::addUserConn(std::shared_ptr<TcpConnection> conn, int userid) {
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

void ChatService::cancelAccount(std::shared_ptr<TcpConnection> conn, const chat::CancelAccountReq& req, chat::CancelAccountRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();

    User user = userModel.query(userid);
    if (user.getId() == -1) {
        res.set_err(1);
        res.set_errmsg("user not exist");
        return;
    }
    friendModel.removeAll(userid);
    friendReqModel.removeAll(userid);
    groupModel.removeAll(userid);
    if (!userModel.remove(userid)) {
        res.set_err(1);
        res.set_errmsg("delete user failed");
        return;
    }
    redis.del("user:state:" + std::to_string(userid));
    removeUser(userid);

    res.set_err(0);
    res.set_errmsg("cancel account success");
}

void ChatService::queryGroupreq(std::shared_ptr<TcpConnection> conn, const chat::QueryGroupReqReq& req, chat::QueryGroupReqRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    std::vector<Group> groups = groupModel.queryGroupId(userid);
    for (auto& g : groups) {
        auto requests = groupReqModel.query(g.getId()); 
        for (auto& r : requests) {
            User user = userModel.query(r.getUserid());
            auto item = res.add_requests();
            item->set_userid(user.getId());
            item->set_username(user.getName());
            item->set_groupid(g.getId());
            item->set_groupname(g.getName());
        }
    }
    res.set_err(0);
    res.set_errmsg("query success");
    LOG_INFO("user {} query group requests", userid);
}

void ChatService::acceptGroup(std::shared_ptr<TcpConnection> conn, const chat::AcceptGroupReq& req, chat::AcceptGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int adminid = conn->getUserId();
    int targetid = req.targetid();

    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isManager(adminid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not admin");
        return;
    }
    if (!groupReqModel.isApplied(targetid, req.groupid())) {
    res.set_err(1);
    res.set_errmsg("application not exist");
    return;
    }

    if (!groupModel.addGroup(targetid, req.groupid(), "normal")) {
        res.set_err(1);
        res.set_errmsg("add group failed");
        return;
    }

    if (!groupReqModel.update(req.groupid(), targetid, 1)) {
        res.set_err(1);
        res.set_errmsg("accept failed");
        return;
    }

    auto target = getUserConn(targetid);
    if (target) {
        chat::GroupAcceptNotify notify;
        notify.set_groupid(req.groupid());
        Group group = groupModel.query(req.groupid());
        notify.set_groupname(group.getName());
        std::string data;
        notify.SerializeToString(&data);
        target->sendMessage(MessageCodec::encode(chat::GROUP_ACCEPT_NOTIFY_MSG, data));
    }
    res.set_err(0);
    res.set_errmsg("accept success");
    LOG_INFO("admin {} accept user {} join group {}", adminid, targetid, req.groupid());
}

void ChatService::queryGroup(std::shared_ptr<TcpConnection> conn, const chat::QueryGroupReq& req, chat::QueryGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();

    std::vector<Group> groups = groupModel.queryGroups(userid);
    for (auto& g : groups) {
        chat::Group* item = res.add_groups();
        item->set_id(g.getId());
        item->set_name(g.getName());
        item->set_desc(g.getDesc());
        item->set_role(groupModel.queryRole(userid, g.getId()));
    }
    res.set_err(0);
    res.set_errmsg("query groups success");
    LOG_INFO("user {} query groups", userid);
}

void ChatService::applyGroup(std::shared_ptr<TcpConnection> conn, const chat::ApplyGroupReq& req, chat::ApplyGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are in group");
        return;
    }
    if (groupReqModel.isApplied(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you applied");
        return;
    }
    
    if (!groupReqModel.insert(req.groupid(), userid)) {
        res.set_err(1);
        res.set_errmsg("apply failed");
        return;
    }
    res.set_err(0);
    res.set_errmsg("apply group success");

    auto managers = groupModel.queryManagers(req.groupid());
    User user = userModel.query(userid);
    Group group = groupModel.query(req.groupid());
    for (auto& manager : managers) {
        auto target = getUserConn(manager.getId());
        if (target) {
            chat::GroupRequest notify;
            notify.set_userid(userid);
            notify.set_username(user.getName());
            notify.set_groupid(req.groupid());
            notify.set_groupname(group.getName());

            std::string data;
            notify.SerializeToString(&data);
            target->sendMessage(MessageCodec::encode(chat::GROUP_NOTIFY_MSG, data));
        }
    }
    LOG_INFO("user {} apply group {} success", userid, req.groupid());
}

void ChatService::leaveGroup(std::shared_ptr<TcpConnection> conn, const chat::LeaveGroupReq& req, chat::LeaveGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (groupModel.isOwner(userid, req.groupid())) {
        res.set_err(2);
        res.set_errmsg("please transfer owner or dissolve group");
        return;
    }
    if (groupModel.leaveGroup(userid, req.groupid())) {
        res.set_err(0);
        res.set_errmsg("leave group success");
        LOG_INFO("user {} leave group {}", userid, req.groupid());

        auto managers = groupModel.queryManagers(req.groupid());
        User user = userModel.query(userid);
        for (auto& manager : managers) {
            auto target = getUserConn(manager.getId());
            if (target) {
                chat::LeaveNotify notify;
                notify.set_userid(userid);
                notify.set_username(user.getName());
                notify.set_groupid(req.groupid());

                std::string data;
                notify.SerializeToString(&data);
                target->sendMessage(MessageCodec::encode(chat::LEAVE_NOTIFY_MSG, data));
            }
        }
    } else {
        res.set_err(1);
        res.set_errmsg("leave group failed");
    }
}

void ChatService::transferOwner(std::shared_ptr<TcpConnection> conn, const chat::TransferOwnerReq& req, chat::TransferOwnerRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int oldownerid = conn->getUserId();
    if (groupModel.isOwner(oldownerid, req.groupid())) {
        if (groupModel.isInGroup(req.newownerid(), req.groupid())) {
            if (groupModel.updateRole(oldownerid, req.groupid(), "normal") && groupModel.updateRole(req.newownerid(), req.groupid(), "owner")) {
                res.set_err(0);
                res.set_errmsg("transfer owner success");
                LOG_INFO("oldowner {} transfer newowner {} success", oldownerid, req.newownerid());
                return;
            }
        }
    }
    res.set_err(1);
    res.set_errmsg("transfer owner failed");
}

void ChatService::dissolveGroup(std::shared_ptr<TcpConnection> conn, const chat::DissolveGroupReq& req, chat::DissolveGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (groupModel.isOwner(userid, req.groupid())) {
        if (groupModel.removeGroup(req.groupid())) {
            res.set_err(0);
            res.set_errmsg("dissolve group success");
            LOG_INFO("owner {} dissolve group {} success", userid, req.groupid());
            return;
        }
    }
    res.set_err(1);
    res.set_errmsg("dissolve group failed");
}

void ChatService::queryGroupUsers(std::shared_ptr<TcpConnection> conn, const chat::QueryGroupUserReq& req, chat::QueryGroupUserRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    auto users = groupModel.queryUsers(req.groupid());
    for (auto& user : users) {
        auto item = res.add_users();
        item->set_userid(user.user.getId());
        item->set_username(user.user.getName());
        item->set_role(user.getRole());
    }
    res.set_err(0);
    res.set_errmsg("query success");
    LOG_INFO("user {} query group {} users", userid, req.groupid());
}

void ChatService::setGroupAdmin(std::shared_ptr<TcpConnection> conn, const chat::SetGroupAdminReq& req, chat::SetGroupAdminRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (!groupModel.isOwner(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not owner");
        return;
    }
    if (!groupModel.isInGroup(req.targetid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("target is not in group");
        return;
    }
    if (groupModel.isManager(req.targetid(), req.groupid())) {
        res.set_err(0);
        res.set_errmsg("target are not normal");
        return;
    }
    if (groupModel.updateRole(req.targetid(), req.groupid(), "admin")) {
        res.set_err(0);
        res.set_errmsg("set group admin success");
        LOG_INFO("owner {} set group {} admin {}", userid, req.groupid(), req.targetid());
        return;
    }
    res.set_err(1);
    res.set_errmsg("set group admin failed");
}

void ChatService::removeGroupAdmin(std::shared_ptr<TcpConnection> conn, const chat::RemoveGroupAdminReq& req, chat::RemoveGroupAdminRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (!groupModel.isOwner(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not owner");
        return;
    }
    if (!groupModel.isInGroup(req.targetid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("target is not in group");
        return;
    }
    if (!groupModel.isAdmin(req.targetid(), req.groupid())) {
        res.set_err(0);
        res.set_errmsg("target are not admin");
        return;
    }
    if (groupModel.updateRole(req.targetid(), req.groupid(), "normal")) {
        res.set_err(0);
        res.set_errmsg("remove group admin success");
        LOG_INFO("owner {} remove group {} admin {}", userid, req.groupid(), req.targetid());
        return;
    }
    res.set_err(1);
    res.set_errmsg("remove group admin failed");
}

void ChatService::removeGroupUser(std::shared_ptr<TcpConnection> conn, const chat::RemoveGroupUserReq& req, chat::RemoveGroupUserRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (!groupModel.isInGroup(req.targetid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("target is not in group");
        return;
    }
    std::string role = groupModel.queryRole(userid, req.groupid());
    if (role == "normal") {
        res.set_err(1);
        res.set_errmsg("you cannot remove users");
        return;
    } else if (role == "admin") {
        if (groupModel.isManager(req.targetid(), req.groupid())) {
            res.set_err(1);
            res.set_errmsg("you cannot remove owner or admin");
            return;
        } else {
            if (groupModel.removeUser(req.targetid(), req.groupid())) {
                auto target = getUserConn(req.targetid());
                if (target) {
                    chat::RemoveGroupUserNotify notify;
                    notify.set_groupid(req.groupid());
                    Group group = groupModel.query(req.groupid());
                    notify.set_groupname(group.getName());
                    std::string data;
                    notify.SerializeToString(&data);
                    target->sendMessage(MessageCodec::encode(chat::REMOVE_GROUP_USER_NOTIFY_MSG, data));
                }
                res.set_err(0);
                res.set_errmsg("remove user success");
                LOG_INFO("admin {} remove user {}", userid, req.targetid());
                return;
            }
            res.set_err(1);
            res.set_errmsg("remove user failed");
        }
    } else {
        if (userid == req.targetid()) {
            res.set_err(1);
            res.set_errmsg("you cannot remove yourself");
            return;
        }
        if (groupModel.removeUser(req.targetid(), req.groupid())) {
            auto target = getUserConn(req.targetid());
            if (target) {
                chat::RemoveGroupUserNotify notify;
                notify.set_groupid(req.groupid());
                Group group = groupModel.query(req.groupid());
                notify.set_groupname(group.getName());
                std::string data;
                notify.SerializeToString(&data);
                target->sendMessage(MessageCodec::encode(chat::REMOVE_GROUP_USER_NOTIFY_MSG, data));
            }
            res.set_err(0);
            res.set_errmsg("remove user success");
            LOG_INFO("owner {} remove user {}", userid, req.targetid());
            return;
        }
        res.set_err(1);
        res.set_errmsg("remove user failed");
    }
}

void ChatService::refuseGroup(std::shared_ptr<TcpConnection> conn, const chat::RefuseGroupReq& req, chat::RefuseGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int adminid = conn->getUserId();
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isManager(adminid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you arenot admin");
        return;
    }
    if (!groupReqModel.isApplied(req.targetid(), req.groupid())) {
    res.set_err(1);
    res.set_errmsg("application not exist");
    return;
    }
    if (!groupReqModel.update(req.groupid(), req.targetid(), 2)) {
        res.set_err(1);
        res.set_errmsg("refuse failed");
        return;
    }

    res.set_err(0);
    res.set_errmsg("refuse success");

    auto target = getUserConn(req.targetid());
    if (target) {
        chat::RefuseGroupNotify notify;
        notify.set_groupid(req.groupid());
        Group group = groupModel.query(req.groupid());
        notify.set_groupname(group.getName());
        std::string data;
        notify.SerializeToString(&data);
        target->sendMessage(MessageCodec::encode(chat::REFUSE_GROUP_NOTIFY_MSG, data));
    }
    
    LOG_INFO("admin {} refuse user {} join group {}", adminid, req.targetid(), req.groupid());
}

void ChatService::addBlacklist(std::shared_ptr<TcpConnection> conn, const chat::BlacklistAddReq& req, chat::BlacklistAddRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    if (userid == req.blackid()) {
        res.set_err(1);
        res.set_errmsg("you cannot black yourself");
        return;
    }
    int blackid = req.blackid();
    User user = userModel.query(blackid);
    if (user.getId() != blackid) {
        res.set_err(1);
        res.set_errmsg("blackid not exist");
        return;
    }
    if (!userModel.isExist(blackid)) {
        res.set_err(1);
        res.set_errmsg("blackid not exist");
        return;
    }
    if (!friendModel.isFriend(userid, blackid)) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }
    if (blacklistModel.isBlacked(userid, blackid)) {
        res.set_err(1);
        res.set_errmsg("have blacked");
        return;
    }
    if (blacklistModel.insert(userid, blackid)) {
        res.set_err(0);
        res.set_errmsg("black success");
        LOG_INFO("user {} black friend {}", userid, blackid);
        return;
    }
    res.set_err(1);
    res.set_errmsg("black failed");
}

void ChatService::removeBlacklist(std::shared_ptr<TcpConnection> conn, const chat::BlacklistRemoveReq& req, chat::BlacklistRemoveRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    int blackid = req.blackid();
    if (!userModel.isExist(blackid)) {
        res.set_err(1);
        res.set_errmsg("blackid not exist");
        return;
    }
    if (!blacklistModel.isBlacked(userid, blackid)) {
        res.set_err(1);
        res.set_errmsg("not blacked");
        return;
    }
    if (blacklistModel.remove(userid, blackid)) {
        res.set_err(0);
        res.set_errmsg("remove black success");
        LOG_INFO("user {} remove black friend {}", userid, blackid);
        return;
    }
    res.set_err(1);
    res.set_errmsg("remove black failed");
}

void ChatService::sendCode(std::shared_ptr<TcpConnection> conn, const chat::SendCodeReq& req, chat::SendCodeRes& res) {
    std::string key;

    if (req.email().empty()) {
        res.set_err(1);
        res.set_errmsg("email empty");
        return;
    }
    key = "verify:email:" + req.email();
    std::string code = VerifyCode::generate();
    
    Redis redis;
    if (!redis.connect()) {
        res.set_err(1);
        res.set_errmsg("redis error");
        return;
    }

    
    if (!redis.setex(key, 300, code)) {
        res.set_err(1);
        res.set_errmsg("save code failed");
        return;
    }
   
   
    EmailSender sender;
    if (!sender.sendCode(req.email(), code)) {
        redis.del(key);
        res.set_err(1);     
        res.set_errmsg("email send failed");
        return;
    }

    res.set_err(0);
    res.set_errmsg("send code success");
}

void ChatService::codeLogin(std::shared_ptr<TcpConnection> conn, const chat::CodeLoginReq& req, chat::CodeLoginRes& res) {
    if (req.email().empty()) {
        res.set_err(1);
        res.set_errmsg("email empty");
        return;
    }
        
    Redis redis;
    if(!redis.connect()) {
        res.set_err(1);
        res.set_errmsg("redis connect failed");
        return;
    }

    std::string verifiedKey = "verified:email:" + req.email() + ":2";
    std::string value;
    if (!redis.get(verifiedKey, value)) {
        res.set_err(1);
        res.set_errmsg("please verify code first");
        return;
    }
    redis.del(verifiedKey);

    User user = userModel.queryByEmail(req.email());
    if (user.getId() == -1) {
        res.set_err(1);
        res.set_errmsg("account not exist");
        return;
    }

    user.setState("online");
    if (!userModel.updateState(user)) {
        res.set_err(1);
        res.set_errmsg("login failed");
        return;
    }

    addUserConn(conn, user.getId());
    conn->setUserId(user.getId());
    
    res.set_err(0);
    res.set_errmsg("login success");
    res.set_userid(user.getId());
    res.set_name(user.getName());
}

void ChatService::verifyCode(std::shared_ptr<TcpConnection> conn, const chat::VerifyCodeReq& req, chat::VerifyCodeRes& res) {
    if (req.email().empty()) {
        res.set_err(1);
        res.set_errmsg("email empty");
        return;
    }
    std::string key = "verify:email:" + req.email();

    Redis redis;
    if (!redis.connect()) {
        res.set_err(1);
        res.set_errmsg("redis error");
        return;
    }

    std::string realCode;
   
    if (!redis.get(key, realCode)) {
        res.set_err(1);
        res.set_errmsg("code expired");
        return;
    }
    
    if (realCode != req.code()) {
        res.set_err(1);
        res.set_errmsg("wrong code");
        return;
    }
    redis.del(key);

    std::string verifiedKey = "verified:email:" + req.email() + ":" + std::to_string(req.scene());
    std::string value = "1";
    if (!redis.setex(verifiedKey, 600, value)) {
        res.set_err(1);
        res.set_errmsg("save verify status failed");
        return;
    }

    res.set_err(0);
    res.set_errmsg("verify success");
}

void ChatService::resetPassword(std::shared_ptr<TcpConnection> conn, const chat::ResetPasswordReq& req, chat::ResetPasswordRes& res) {
    if (req.email().empty() || req.newpassword().empty()) {
        res.set_err(1);
        res.set_errmsg("email or newpassword empty");
        return;
    }

    Redis redis;
    if (!redis.connect()) {
        res.set_err(1);
        res.set_errmsg("redis connect failed");
        return;
    }
    std::string verifiedKey = "verified:email:" + req.email() + ":3";
    std::string value;
    if (!redis.get(verifiedKey, value)) {
        res.set_err(1);
        res.set_errmsg("please verify code first");
        return;
    }
    redis.del(verifiedKey);

    User user = userModel.queryByEmail(req.email());
    if (user.getId() == -1) {
        res.set_err(1);
        res.set_errmsg("account not exist");
        return;
    }
    if (!userModel.updatePassword(user.getId(), req.newpassword())) {
        res.set_err(1);
        res.set_errmsg("reset password failed");
        return;
    }
    res.set_err(0);
    res.set_errmsg("reset password success");
}

void ChatService::queryFile(std::shared_ptr<TcpConnection> conn, const chat::QueryFileReq& req, chat::QueryFileRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    std::vector<File> files = fileModel.queryOffline(userid);
    std::vector<Group> groups = groupModel.queryGroups(userid);
    if (!groups.empty()) {
        for (auto& g : groups) {
            std::vector<File> file = fileModel.queryGroupFile(g.getId());
            files.insert(files.end(), file.begin(), file.end());
        }
    }
    if (files.empty()) {
        res.set_err(1);
        res.set_errmsg("query file empty");
        return;
    }
    for (const auto& f : files) {
        auto* info = res.add_files();
        info->set_fileid(f.getFileid());
        info->set_filename(f.getFilename());
        info->set_filesize(f.getFilesize());
        info->set_fromid(f.getFromid());
        info->set_fromname(f.getFromname());
        info->set_groupid(f.getGroupid());
    }
    res.set_err(0);
    res.set_errmsg("query file success");
}

void ChatService::checkFriend(std::shared_ptr<TcpConnection> conn, const chat::CheckFriendReq& req, chat::CheckFriendRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    int friendid = req.friendid();
    if (!userModel.isExist(friendid)) {
        res.set_err(1);
        res.set_errmsg("friendid not exist");
        return;
    }
    if (!friendModel.isFriend(userid, friendid)) {
        res.set_err(1);
        res.set_errmsg("not friend");
        return;
    }
    if (blacklistModel.isBlacked(userid, friendid) || blacklistModel.isBlacked(friendid, userid)) {
        res.set_err(1);
        res.set_errmsg("message blacked");
        return;
    }
}

void ChatService::checkGroup(std::shared_ptr<TcpConnection> conn, const chat::CheckGroupReq& req, chat::CheckGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    int userid = conn->getUserId();
    int groupid = req.groupid();
    if (!groupModel.isGroupExist(groupid)) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(userid, groupid)) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    } 
}
