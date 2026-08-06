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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.fromid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }

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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }

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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }

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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }

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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.fromid() && conn->getUserId() != req.toid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.fromid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }

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
        messageModel.insert(req.fromid(), req.toid(), req.msg(), req.fromname());
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

        messageModel.insert(req.fromid(), req.toid(), req.msg(), req.fromname());

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
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
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

void ChatService::groupChat(std::shared_ptr<TcpConnection> conn, const chat::GroupChatReq& req, chat::GroupChatRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if(!groupMessageModel.insert(req.groupid(), req.userid(), req.msg(), req.username())) {
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
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
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
    res.set_err(0);
    res.set_errmsg("query group history success");
    LOG_INFO("query group {} history message", req.groupid());
}

void ChatService::fileStart(std::shared_ptr<TcpConnection> conn, const chat::FileStartReq& req, chat::FileStartRes& res) {
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
        info.fromid = req.fromid();
        info.toid = req.toid();
        info.filename = req.filename();
        info.filesize = req.filesize();
        info.fileid = req.fileid();
        info.path = path;
        fileInfoMap[req.fileid()] = info;
    }

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
    }
    LOG_INFO("receive file {} finish", req.fileid());

    auto target = getUserConn(info.toid);
    if (target) sendFile(target, info);
    else {
        File file;
        file.setFromid(info.fromid);
        file.setToid(info.toid);
        file.setFilename(info.filename);
        file.setFilesize(info.filesize);
        file.setFileid(info.fileid);
        file.setStatus(0);
        fileModel.insert(file);

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

    res.set_err(0);
    res.set_errmsg("upload success");
}

void ChatService::sendFile(std::shared_ptr<TcpConnection> conn, const FileInfo& info) {
    FILE* fp = fopen(info.path.c_str(), "rb");
    if (fp == nullptr) return;

    chat::FileStartReq start;
    start.set_fromid(info.fromid);
    start.set_toid(info.toid);
    start.set_filename(info.filename);
    start.set_filesize(info.filesize);
    start.set_fileid(info.fileid);

    std::string data;
    start.SerializeToString(&data);
    conn->sendMessage(MessageCodec::encode(chat::FILE_START_MSG, data));

    char buffer[64 * 1024];
    uint64_t offset = 0;
    while(true) {
        int n = fread(buffer, 1, sizeof(buffer), fp);
        if (n <= 0) break;
        chat::FileChunkReq chunk;
        chunk.set_fileid(info.fileid);
        chunk.set_offset(offset);
        chunk.set_data(buffer, n);
        data.clear();
        chunk.SerializeToString(&data);
        conn->sendMessage(MessageCodec::encode(chat::FILE_CHUNK_MSG, data));
        offset += n;
    }
    fclose(fp);
    chat::FileEndReq end;
    end.set_fileid(info.fileid);
    data.clear();
    end.SerializeToString(&data);
    conn->sendMessage(MessageCodec::encode(chat::FILE_END_MSG, data));
}

void ChatService::downloadFile(std::shared_ptr<TcpConnection> conn, const chat::DownloadFileReq& req, chat::DownloadFileRes& res) {
    File file = fileModel.queryByFileid(req.fileid());
    if (file.getFileid().empty()) {
        res.set_err(1);
        res.set_errmsg("file not exist");
        return;
    }
    
    std::string path = "./files/" + req.fileid();
    FILE* fp = fopen(path.c_str(), "rb");
    if (fp == nullptr) {
        res.set_err(1);
        res.set_errmsg("file not exist");
        return;
    }

    chat::DownloadStart start;
    start.set_fileid(req.fileid());
    start.set_filename(file.getFilename());
    start.set_filesize(file.getFilesize());

    std::string data;
    start.SerializeToString(&data);
    conn->sendMessage(MessageCodec::encode(chat::DOWNLOAD_START_MSG, data));

    char buffer[64 * 1024];
    uint64_t offset = 0;
    while(true) {
        int n = fread(buffer, 1, sizeof(buffer), fp);
        if (n <= 0) break;
        chat::DownloadChunk chunk;
        chunk.set_fileid(req.fileid());
        chunk.set_offset(offset);
        chunk.set_data(buffer, n);
        data.clear();
        chunk.SerializeToString(&data);
        conn->sendMessage(MessageCodec::encode(chat::DOWNLOAD_CHUNK_MSG, data));
        offset += n;
    }
    fclose(fp);
    chat::DownloadEnd end;
    end.set_fileid(req.fileid());
    data.clear();
    end.SerializeToString(&data);
    conn->sendMessage(MessageCodec::encode(chat::DOWNLOAD_END_MSG, data));
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
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
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
    int userid = req.userid();
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
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
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isManager(userid, req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you arenot admin");
        return;
    }

    auto group = groupModel.query(req.groupid());
    auto requests = groupReqModel.query(req.groupid());
    for (auto& r : requests) {
        User user = userModel.query(r.getUserid());
        auto item = res.add_requests();
        item->set_userid(user.getId());
        item->set_username(user.getName());
        item->set_groupid(group.getId());
        item->set_groupname(group.getName());
    }
    res.set_err(0);
    res.set_errmsg("query success");
    LOG_INFO("user {} query group {} requests", userid, req.groupid());
}

void ChatService::acceptGroup(std::shared_ptr<TcpConnection> conn, const chat::AcceptGroupReq& req, chat::AcceptGroupRes& res) {
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
    if (!groupReqModel.isApplied(req.userid(), req.groupid())) {
    res.set_err(1);
    res.set_errmsg("application not exist");
    return;
    }

    if (!groupModel.addGroup(req.userid(), req.groupid(), "normal")) {
        res.set_err(1);
        res.set_errmsg("add group failed");
        return;
    }

    if (!groupReqModel.update(req.groupid(), req.userid(), 1)) {
        res.set_err(1);
        res.set_errmsg("accept failed");
        return;
    }

    auto target = getUserConn(req.userid());
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
    LOG_INFO("admin {} accept user {} join group {}", adminid, req.userid(), req.groupid());
}

void ChatService::queryGroup(std::shared_ptr<TcpConnection> conn, const chat::QueryGroupReq& req, chat::QueryGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }

    std::vector<Group> groups = groupModel.queryGroups(req.userid());
    for (auto& g : groups) {
        chat::Group* item = res.add_groups();
        item->set_id(g.getId());
        item->set_name(g.getName());
        item->set_desc(g.getDesc());
        item->set_role(groupModel.queryRole(req.userid(), g.getId()));
    }
    res.set_err(0);
    res.set_errmsg("query groups success");
    LOG_INFO("user {} query groups", req.userid());
}

void ChatService::applyGroup(std::shared_ptr<TcpConnection> conn, const chat::ApplyGroupReq& req, chat::ApplyGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (groupModel.isInGroup(req.userid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are in group");
        return;
    }
    if (groupReqModel.isApplied(req.userid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you applied");
        return;
    }
    
    if (!groupReqModel.insert(req.groupid(), req.userid())) {
        res.set_err(1);
        res.set_errmsg("apply failed");
        return;
    }
    res.set_err(0);
    res.set_errmsg("apply group success");

    auto managers = groupModel.queryManagers(req.groupid());
    User user = userModel.query(req.userid());
    Group group = groupModel.query(req.groupid());
    for (auto& manager : managers) {
        auto target = getUserConn(manager.getId());
        if (target) {
            chat::GroupRequest notify;
            notify.set_userid(req.userid());
            notify.set_username(user.getName());
            notify.set_groupid(req.groupid());
            notify.set_groupname(group.getName());

            std::string data;
            notify.SerializeToString(&data);
            target->sendMessage(MessageCodec::encode(chat::GROUP_NOTIFY_MSG, data));
        }
    }
    LOG_INFO("user {} apply group {} success", req.userid(), req.groupid());
}

void ChatService::leaveGroup(std::shared_ptr<TcpConnection> conn, const chat::LeaveGroupReq& req, chat::LeaveGroupRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(req.userid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (groupModel.isOwner(req.userid(), req.groupid())) {
        res.set_err(2);
        res.set_errmsg("please transfer owner or dissolve group");
        return;
    }
    if (groupModel.leaveGroup(req.userid(), req.groupid())) {
        res.set_err(0);
        res.set_errmsg("leave group success");
        LOG_INFO("user {} leave group {}", req.userid(), req.groupid());
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
    if (groupModel.isOwner(req.oldownerid(), req.groupid())) {
        if (groupModel.isInGroup(req.newownerid(), req.groupid())) {
            if (groupModel.updateRole(req.oldownerid(), req.groupid(), "normal") && groupModel.updateRole(req.newownerid(), req.groupid(), "owner")) {
                res.set_err(0);
                res.set_errmsg("transfer owner success");
                LOG_INFO("oldowner {} transfer newowner {} success", req.oldownerid(), req.newownerid());
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
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (!groupModel.isGroupExist(req.groupid())) {
        res.set_err(1);
        res.set_errmsg("group not exist");
        return;
    }
    if (!groupModel.isInGroup(req.userid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (groupModel.isOwner(req.userid(), req.groupid())) {
        if (groupModel.removeGroup(req.groupid())) {
            res.set_err(0);
            res.set_errmsg("dissolve group success");
            LOG_INFO("owner {} dissolve group {} success", req.userid(), req.groupid());
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
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (!groupModel.isInGroup(req.userid(), req.groupid())) {
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
    LOG_INFO("user {} query group {} users", req.userid(), req.groupid());
}

void ChatService::setGroupAdmin(std::shared_ptr<TcpConnection> conn, const chat::SetGroupAdminReq& req, chat::SetGroupAdminRes& res) {
    if (conn->getUserId() <= 0) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (!groupModel.isInGroup(req.userid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (!groupModel.isOwner(req.userid(), req.groupid())) {
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
        LOG_INFO("owner {} set group {} admin {}", req.userid(), req.groupid(), req.targetid());
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
    if (conn->getUserId() != req.userid()) {
        res.set_err(1);
        res.set_errmsg("please login first");
        return;
    }
    if (!groupModel.isInGroup(req.userid(), req.groupid())) {
        res.set_err(1);
        res.set_errmsg("you are not in group");
        return;
    }
    if (!groupModel.isOwner(req.userid(), req.groupid())) {
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
        LOG_INFO("owner {} remove group {} admin {}", req.userid(), req.groupid(), req.targetid());
        return;
    }
    res.set_err(1);
    res.set_errmsg("remove group admin failed");
}