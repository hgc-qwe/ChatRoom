#include <iostream>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <limits>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <atomic>
#include <poll.h>
#include <cerrno>
#include <mutex>
#include "Logger.h"
#include "Buffer.h"
#include "MessageCodec.h"
#include "chat.pb.h"
#include "Util.h"
using namespace std;

int currentUserid = -1;
ofstream recvFile;
string recvFileId;
uint64_t recvFileSize = 0;
ofstream downloadFile;
string downloadFileId;
string downloadFilePath;
atomic<bool> downloadActive{false};
bool verifyCodeFinished = false;
bool verifyCodeSuccess = false;
bool checkFriendFinished = false;
bool checkFriendSuccess = false;
bool checkGroupFinished = false;
bool checkGroupSuccess = false;
atomic<bool> running{true};
SSL* ssl = nullptr;
SSL_CTX* sslCtx = nullptr;
vector<std::thread> fileThreads;
mutex sslWriteMutex;
bool needSend = true;

std::string makeDownloadPath(const string& filename) {
    namespace fs = filesystem;

    const fs::path directory{"./clientDownload"};
    fs::create_directories(directory);

    fs::path name = fs::path(filename).filename();
    if (name.empty()) name = "download";

    fs::path candidate = directory / name;
    for (unsigned int index = 1; fs::exists(candidate); ++index) {
        candidate = directory / (name.stem().string() + "(" + to_string(index) + ")" + name.extension().string());
    }
    return candidate.string();
}

bool sendAll(SSL* ssl, const char* data, size_t len) {
    lock_guard<mutex> lock(sslWriteMutex);

    size_t sent = 0;
    while (sent < len) {
        int n = SSL_write(ssl, data + sent, len - sent);
        if (n <= 0) {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
            ERR_print_errors_fp(stderr);
            return false;
        }
        sent += n;
    }
    return true;
}

void verifyCode(SSL* ssl, const string& email, const string& code, int scene) {
    chat::VerifyCodeReq req;
    req.set_email(email);
    req.set_code(code);
    req.set_scene(scene);

    string data;
    req.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::VERIFY_CODE_MSG, data);

    sendAll(ssl, packet.data(), packet.size());
}

void checkFriend(SSL* ssl, const int friendid) {
    chat::CheckFriendReq req;
    req.set_friendid(friendid);

    string data;
    req.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::CHECK_FRIEND_MSG, data);

    sendAll(ssl, packet.data(), packet.size());
}

void checkGroup(SSL* ssl, const int groupid) {
    chat::CheckGroupReq req;
    req.set_groupid(groupid);

    string data;
    req.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::CHECK_GROUP_MSG, data);

    sendAll(ssl, packet.data(), packet.size());
}

void sendCode(SSL* ssl, const string& email, int scene) {
    chat::SendCodeReq req;
    req.set_email(email);
    req.set_scene(scene);

    string data;
    req.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::SEND_CODE_MSG, data);

    sendAll(ssl, packet.data(), packet.size());
}

void queryFriend(SSL* ssl) {
    chat::QueryFriendReq req;
    req.set_msgid(chat::QUERY_FRIEND_MSG);

    string data;
    req.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::QUERY_FRIEND_MSG, data);

    sendAll(ssl, packet.data(), packet.size());
}

void sendFile(SSL* ssl, int toid, int groupid, const string& path) {
    namespace fs = filesystem;
    
    ifstream ifs(path, ios::binary);
    if (!ifs.is_open()) {
        cout << "open file failed" << endl;
        return;
    }
    
    uint64_t filesize = fs::file_size(path);
    string filename = fs::path(path).filename().string();
    string fileid = to_string(chrono::system_clock::now().time_since_epoch().count());

    chat::FileStartReq startReq;
    startReq.set_toid(toid);
    startReq.set_filename(filename);
    startReq.set_filesize(filesize);
    startReq.set_fileid(fileid);
    startReq.set_groupid(groupid);

    string data;
    startReq.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::FILE_START_MSG, data);

    if (!sendAll(ssl, packet.data(), packet.size())) return;

    
    const size_t CHUNK_SIZE = 64 * 1024;

    char buffer[CHUNK_SIZE];

    while (ifs) {
        ifs.read(buffer, CHUNK_SIZE);
        streamsize len = ifs.gcount();
        if (len <= 0) break;

        chat::FileChunkReq chunkReq;
        chunkReq.set_fileid(fileid);
        chunkReq.set_data(buffer, len);

        data.clear();
        chunkReq.SerializeToString(&data);
        packet = MessageCodec::encode(chat::FILE_CHUNK_MSG, data);

        if (!sendAll(ssl, packet.data(), packet.size())) {
            cout << "file send failed" << endl;
            return;
        }
    }

    chat::FileEndReq endReq;
    endReq.set_fileid(fileid);

    data.clear();
    endReq.SerializeToString(&data);
    packet = MessageCodec::encode(chat::FILE_END_MSG, data);

    if (!sendAll(ssl, packet.data(), packet.size())) {
        cout << "file end send failed" << endl;
        return;
    }

    cout << "file send finish" << endl;
}

void queryFile() {
    chat::QueryFileReq req;
    req.set_msgid(chat::QUERY_FILE_MSG);

    string data;
    req.SerializeToString(&data);
    string packet = MessageCodec::encode(chat::QUERY_FILE_MSG, data);

    sendAll(ssl, packet.data(), packet.size());
}

void recvMessage(SSL* ssl) {
    Buffer buf;
    int fd = SSL_get_fd(ssl);

    while (running) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int ret = poll(&pfd, 1, 100);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        if (ret == 0) continue;

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            cout << "socket colse or error" << endl;
            break;
        }
        if (!(pfd.revents & POLLIN)) continue;

        char buffer[64 * 1024] = {0};
        int n = SSL_read(ssl, buffer, sizeof(buffer));
        if (n > 0) {
            buf.append(buffer, n);
        } else {
            int err = SSL_get_error(ssl, n);
            if (err == SSL_ERROR_ZERO_RETURN) {
                cout << "server close" << endl;
                break;
            }
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
            ERR_print_errors_fp(stderr);
            cout << "SSL_read filed" << endl;
            running = false;
            break;
        }

        int msgid;
        string body;
        while (MessageCodec::decode(buf, msgid, body)) {
            if(msgid == chat::LOGIN_MSG_ACK) {
                chat::LoginRes res;
                res.ParseFromString(body);

                if (res.err() == 1) cout << "login:" << res.errmsg() << endl;

                if(res.offlinemsgs_size() > 0) {
                    cout << "\n===== 离线私聊消息 =====" << endl;
                    for(auto &msg : res.offlinemsgs()) cout << "[" << msg.time() << "] " << msg.fromid() << " -> " << msg.toid() << " : " << msg.msg() << endl;
                    
                }
                if(res.offlinegroupmsg_size() > 0) {
                    cout << "\n===== 离线群聊消息 =====" << endl;
                    for(auto &msg : res.offlinegroupmsg()) cout << "[" << msg.time() << "] " << "group " << msg.groupid() << " user " << msg.userid() << " : " << msg.msg() << endl;
                    
                }

                if(res.offlinefiles_size() > 0) {
                    cout << "\n===== 离线文件消息 =====" << endl;
                    for(auto& file : res.offlinefiles()) cout <<"离线文件:" <<file.filename() <<" size:" <<file.filesize() <<" from:" <<file.fromid() <<" fileid:" <<file.fileid() <<endl;
                    
                }
                if (res.offlinegroupfiles_size() > 0) {
                    for (int i = 0; i < res.offlinegroupfiles_size(); ++i) {
                        const auto& file = res.offlinegroupfiles(i);
                        cout << "\n========== 离线群文件 ==========" << endl;
                        cout << "群组: " << file.groupname() << endl;
                        cout << "发送者: " << file.fromname() << " (userid=" << file.fromid() << ")" << endl;
                        cout << "文件名: " << file.filename() << endl;
                        cout << "文件大小: " << file.filesize() << " bytes" << endl;
                        cout << "fileid: " << file.fileid() << endl;
                        cout << "================================" << endl;
                    }
                }
                if (res.offlinefriendrequests_size() > 0) {
                    for (const auto& request : res.offlinefriendrequests())
                    {
                        cout << "\n========== 离线好友申请 ==========" << endl;
                        cout << "userid：" << request.userid() << " username：" << request.username() << endl;
                    }
                }
            }
            else if(msgid == chat::ADD_FRIEND_MSG_ACK) {
                chat::AddFriendRes res;
                res.ParseFromString(body);

                cout << "add friend:" << res.errmsg() << endl;
            }
            else if(msgid == chat::QUERY_FRIEND_REQ_MSG_ACK) {
                chat::QueryFriendReqRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                for(auto& r : res.requests()) cout << "request userid:" << r.userid() << " username:" << r.username() << endl;
                
            }
            else if(msgid == chat::ACCEPT_FRIEND_MSG_ACK) {
                chat::AcceptFriendRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::FRIEND_NOTIFY_MSG) {
                chat::FriendRequest req;
                req.ParseFromString(body);

                cout << "收到好友申请:" << " userid=" << req.userid() << " username=" << req.username() << endl;
            }
            else if(msgid == chat::FRIEND_ACCEPT_NOTIFY_MSG) {
                chat::FriendAcceptNotify req;
                req.ParseFromString(body);

                cout << "好友申请通过:" << " userid=" << req.userid() << " username=" << req.username() << endl;
                cout << endl;
                queryFriend(ssl);
            }
            else if(msgid == chat::QUERY_FRIEND_MSG_ACK) {
                chat::QueryFriendRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                for(auto& user : res.friends()) cout << "friend id:" << user.id() << " name:" << user.name() << " state:" << user.state() << endl;
            }
            else if(msgid == chat::DELETE_FRIEND_MSG_ACK) {
                chat::DeleteFriendRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::HISTORY_MSG_ACK) {
                chat::HistoryMsgRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                for(auto& msg : res.msgs()) cout << "from:" << msg.fromid() << " -> " << "to:" << msg.toid() << " msg:"<< msg.msg() << right<< setw(10) << "time:" << msg.time() << endl;
                for (auto& f : res.files()) cout << "from:" << f.fromid() << " -> " << "to:" << f.toid() << " fileid:" << f.fileid() << " filename:" << f.filename() << " filesize:" << f.filesize() << endl;
            }
            else if(msgid == chat::ONE_CHAT_MSG) {
                chat::OneChatNotify notify;
                if (!notify.ParseFromString(body)) {
                    cout << "OneChatNotify parse failed" << endl;
                    continue;
                }

                cout << notify.fromname() << ": " << endl;
                cout << notify.msg() << endl;
            }
            else if(msgid == chat::GROUP_CHAT_MSG) {
                chat::GroupChatNotify notify;
                notify.ParseFromString(body);

                cout << "group " << notify.groupid() << " " << notify.username() << ": " << endl;
                cout << notify.msg() << endl;
            }
            else if(msgid == chat::GROUP_HISTORY_MSG_ACK) {
                chat::GroupHistoryMsgRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                for(auto& msg : res.msgs()) cout << "group:" << msg.groupid() << "     " << "user:" << msg.userid() << " msg:" << msg.msg() << right << setw(10) << "time:" << msg.time() << endl;
                for (auto& f : res.files()) cout << "group:" << f.groupid() << "     from:" << f.userid() << " fileid:" << f.fileid() << " filename:" << f.filename() << " filesize:" << f.filesize() << endl;
            }
            else if(msgid == chat::FILE_START_MSG) {
                chat::FileStartReq req;
                req.ParseFromString(body);
                std::string path = makeDownloadPath(req.filename());

                recvFile.open(path, std::ios::binary);
                if(recvFile.is_open()) {
                    recvFileId = req.fileid();
                    recvFileSize = req.filesize();                  
                }
            }
            else if(msgid == chat::FILE_CHUNK_MSG) {
                chat::FileChunkReq req;
                req.ParseFromString(body);

                if(recvFile.is_open() && req.fileid() == recvFileId) {
                    recvFile.write(req.data().data(), req.data().size());
                }
            }
            else if(msgid == chat::FILE_END_MSG) {
                chat::FileEndReq req;
                req.ParseFromString(body);

                if(recvFile.is_open() && req.fileid() == recvFileId) {
                    recvFile.close();
                }
            }
            else if(msgid == chat::DOWNLOAD_START_MSG) {
                chat::DownloadStart start;
                if (!start.ParseFromString(body)) {
                    cout << "DOWNLOAD_START parse failed" << endl;
                    continue;
                }

                cout << "开始下载:" << start.filename() << " size:" << start.filesize() << endl;

                downloadFilePath = makeDownloadPath(start.filename());
                downloadFile.open(downloadFilePath, std::ios::binary);
                if(downloadFile.is_open()) {
                    downloadFileId = start.fileid();
                    downloadActive = true;
                    cout << "保存到:" << downloadFilePath << endl;
                } else cout << "open download file failed" << endl;
            }
            else if(msgid == chat::DOWNLOAD_CHUNK_MSG) {
                chat::DownloadChunk chunk;
                if (!chunk.ParseFromString(body)) {
                    cout << "DOWNLOAD_CHUNK parse failed" << endl;
                    continue;
                }

                if(downloadFile.is_open() && chunk.fileid() == downloadFileId) {
                    downloadFile.write( chunk.data().data(), chunk.data().size());
                }
                if (!downloadFile) {
                    cout << "write download file failed" << endl;
                    downloadFile.close();
                    downloadActive = false;
                }
            }
            else if(msgid == chat::DOWNLOAD_END_MSG) {
                chat::DownloadEnd end;
                if (!end.ParseFromString(body)) {
                    cout << "DOWNLOAD_END parse failed" << endl;
                    continue;
                }

                if(downloadFile.is_open() && end.fileid() == downloadFileId) {
                    downloadFile.close();
                    cout << "下载完成:" << downloadFilePath << endl;
                    downloadActive = false;
                    downloadFileId.clear();
                }
            }
            else if(msgid == chat::CANCEL_ACCOUNT_MSG_ACK) {
                chat::CancelAccountRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                if(res.err() == 0) {
                    running = false;
                    SSL_shutdown(ssl);
                    return;
                }
            }
             else if(msgid == chat::REG_MSG_ACK){
                chat::RegisterRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                if(res.err() == 0) {
                    cout << "userid: " << res.userid() << endl;
                }
            }
            else if(msgid == chat::GROUP_NOTIFY_MSG) {
                chat::GroupRequest req;
                req.ParseFromString(body);

                cout << "收到加群申请:" << " userid=" << req.userid() << " username=" << req.username() << " groupid=" << req.groupid() << " groupname=" << req.groupname() << endl;
            }
            else if(msgid == chat::GROUP_ACCEPT_NOTIFY_MSG) {
                chat::GroupAcceptNotify notify;
                notify.ParseFromString(body);

                cout << "加入群成功:" << " groupid=" << notify.groupid() << " groupname=" << notify.groupname() << endl;
            }
            else if(msgid == chat::QUERY_GROUP_REQ_MSG_ACK) {
                chat::QueryGroupReqRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                for(auto& r:res.requests()) {
                    cout << "userid:" << r.userid() << " username:" << r.username() << " groupid:" << r.groupid() << " groupname:" << r.groupname() << endl;
                }
            }
            else if(msgid == chat::LEAVE_GROUP_MSG_ACK) {
                chat::LeaveGroupRes res;
                res.ParseFromString(body);

                if (res.err() == 1) cout << res.errmsg() << endl;
                if(res.err() == 2) {
                    cout << "\nYou are owner.\n";
                    cout << "18 转让群主\n";
                    cout << "19 解散群\n";
                }
            }
            else if(msgid == chat::TRANSFER_OWNER_MSG_ACK) {
                chat::TransferOwnerRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::DISSOLVE_GROUP_MSG_ACK) {
                chat::DissolveGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::QUERY_GROUP_USER_MSG_ACK) {
                chat::QueryGroupUserRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;

                for(auto &u : res.users()) {
                    cout << "userid:" << u.userid() << " username:" << u.username() << " role:" << u.role() << endl;
                }
            }
            else if(msgid == chat::SET_GROUP_ADMIN_MSG_ACK) {
                chat::SetGroupAdminRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::REMOVE_GROUP_ADMIN_MSG_ACK){
                chat::RemoveGroupAdminRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::REMOVE_GROUP_USER_MSG_ACK) {
                chat::RemoveGroupUserRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if (msgid == chat::REMOVE_GROUP_USER_NOTIFY_MSG) {
                chat::RemoveGroupUserNotify notify;
                notify.ParseFromString(body);

                cout << "你已被移出群：" << notify.groupname() << " (groupid = " << notify.groupid() << ")" << endl;
            }
            else if(msgid == chat::REFUSE_GROUP_MSG_ACK) {
                chat::RefuseGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::REFUSE_GROUP_NOTIFY_MSG) {
                chat::RefuseGroupNotify notify;
                notify.ParseFromString(body);

                cout << "你的加群申请被拒绝:" << " groupid=" << notify.groupid() << " groupname=" << notify.groupname() << endl;
            }
            else if (msgid == chat::GROUP_FILE_NOTIFY_MSG) {
                chat::GroupFileNotify notify;
                if (!notify.ParseFromString(body)) {
                    cout << "GroupFileNotify parse failed" << endl;
                    return;
                }

                cout << "\n========== 收到群文件 ==========" << endl;
                cout << "群组: " << notify.groupname() << endl;
                cout << "发送者: " << notify.fromname() << " (userid=" << notify.fromid() << ")" << endl;
                cout << "文件名: " << notify.filename() << endl;
                cout << "文件大小: " << notify.filesize() << " bytes" << endl;
                cout << "fileid: " << notify.fileid() << endl;
                cout << "================================" << endl;
            }
            else if (msgid == chat::BLACKLIST_ADD_MSG_ACK) {
                chat::BlacklistAddRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if (msgid == chat::BLACKLIST_REMOVE_MSG_ACK) {
                chat::BlacklistRemoveRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }
            else if(msgid == chat::SEND_CODE_MSG_ACK) {
                chat::SendCodeRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;

            }
            else if(msgid == chat::VERIFY_CODE_MSG_ACK) {
                chat::VerifyCodeRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;

                if(res.err() == 0) verifyCodeSuccess = true;
                else verifyCodeSuccess = false;
                verifyCodeFinished = true;
            }
            else if (msgid == chat::CODE_LOGIN_MSG_ACK) {
                chat::CodeLoginRes res;
                res.ParseFromString(body);

                if (res.err() != 0) {
                    cout << "登录失败：" << res.errmsg() << endl;
                    continue;
                }
                cout << "登录成功" << endl;
                cout << "userid: " << res.userid() << endl;
                cout << "username: " << res.name() << endl;
            }
            else if (msgid == chat::RESET_PASSWORD_MSG_ACK) {
                chat::ResetPasswordRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
            }

            else if (msgid == chat::PING_MSG) {
                std::string packet = MessageCodec::encode(chat::PONG_MSG, "");
                sendAll( ssl, packet.data(), packet.size());

                // cout << "[Client] recv PING, send PONG" << endl;
            }
            else if(msgid == chat::QUERY_GROUP_MSG_ACK) {
                chat::QueryGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                for(auto& r:res.groups()) {
                    cout << "groupid:" << r.id() << " groupname:" << r.name() << " groupdesc:" << r.desc() << " your role:" << r.role() << endl;
                }
            }
            else if(msgid == chat::QUERY_FILE_MSG_ACK) {
                chat::QueryFileRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                for(auto& r:res.files()) {
                    cout << "fileid:" << r.fileid() << " filename:" << r.filename() << " filesize:" << r.filesize() << " fromid:" << r.fromid() << " fromname:" << r.fromname() << endl;
                }
            }
            else if(msgid == chat::CHECK_FRIEND_MSG_ACK) {
                chat::CheckFriendRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;

                if(res.err() == 0) checkFriendSuccess = true;
                else checkFriendSuccess = false;
                checkFriendFinished = true;
            }
            else if(msgid == chat::CHECK_GROUP_MSG_ACK) {
                chat::CheckGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;

                if(res.err() == 0) checkGroupSuccess = true;
                else checkGroupSuccess = false;
                checkGroupFinished = true;
            }
            else if(msgid == chat::CREATE_GROUP_MSG_ACK) {
                chat::CreateGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                cout << "groupid:" << res.groupid() << endl;
            }
            else if(msgid == chat::ACCEPT_GROUP_MSG_ACK) {
                chat::AcceptGroupRes res;
                res.ParseFromString(body);

                cout << res.errmsg() << endl;
                
            }
            else if(msgid == chat::INVITE_NOTIFY_MSG) {
                chat::InviteNotify notify;
                notify.ParseFromString(body);

                cout << "你已被拉进群聊：" << "ownerid:" << notify.ownerid() << " groupid:" << notify.groupid() << " groupname:" << notify.groupname() << endl;
            }
            else if(msgid == chat::LEAVE_NOTIFY_MSG) {
                chat::LeaveNotify notify;
                notify.ParseFromString(body);

                cout << notify.username() << " id:" << notify.userid() << " 已退出群聊groupid:" << notify.groupid() << endl;
            }
            else if (msgid == chat::FILE_NOTIFY_MSG) {
                chat::FileNotify notify;
                if (!notify.ParseFromString(body)) {
                    cout << "FileNotify parse failed" << endl;
                    continue;
                }

                cout << "\n========== 收到私聊文件 ==========" << endl;
                cout << "发送者: " << notify.fromname()
                    << " (userid=" << notify.fromid() << ")" << endl;
                cout << "文件名: " << notify.filename() << endl;
                cout << "文件大小: " << notify.filesize() << " bytes" << endl;
                cout << "fileid: " << notify.fileid() << endl;
                cout << "请输入 11 并使用 fileid 下载" << endl;
                cout << "==================================" << endl;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    string serverIp = "127.0.0.1";
    int serverPort = 8000;

    if (argc >= 2) serverIp = argv[1];
    if (argc == 3) {
        try {
            serverPort = stoi(argv[2]);
        } catch(...) {
            cerr << "invalid port: " << argv[2] << endl;
            close(sockfd);
            return -1;
        }
    }
    if (argc >= 4) {
        cerr << "invailed ip : port" << endl;
        close(sockfd);
        return -1;
    }

    if (serverPort < 1 || serverPort > 65535) {
        cerr << "invalid port: " << serverPort << endl;
        close(sockfd);
        return -1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIp.c_str(), &server.sin_addr) <= 0) {
        cerr << "invalid server ip: " << serverIp << endl;
        close(sockfd);
        return -1;
    }

    cout << "server address: " << serverIp << ":" << serverPort << endl;

    if (connect(sockfd, (sockaddr*)&server, sizeof(server)) < 0) {
        cout << "connect failed" << endl;
        return -1;
    }
    cout << "connect success" << endl;

    sslCtx = SSL_CTX_new(TLS_client_method());
    if (!sslCtx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    ssl = SSL_new(sslCtx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(sslCtx);
        return -1;
    }

    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(sslCtx);
        close(sockfd);
        return -1;
    }
    cout << "TLS handshake success" << endl;
    running = true;

    thread recvThread(recvMessage, ssl);

    while(true) {
        cout << "\n============================ menu =============================\n";
        cout << "[账号与认证]\n";
        cout << "1 登录                 12 注销账户              13 注册\n";
        cout << "28 验证码登录          29 重置密码\n";
        cout << "---------------------------------------------------------------\n";
        cout << "[好友管理]\n";
        cout << "2 添加好友             3 查看好友申请           4 同意好友申请  \n";
        cout << "5 删除好友             26 添加到黑名单          27 从黑名单移出\n";
        cout << "30 查询好友\n";
        cout << "---------------------------------------------------------------\n";
        cout << "[群组管理]\n";
        cout << "14 申请加群            15 查看群申请            16 同意入群\n";
        cout << "17 退出群              18 转让群主              19 解散群 \n";
        cout << "20 查看群成员          21 设置管理员            22 删除管理员\n";
        cout << "23 移除成员            24 拒绝入群              31 查询所在群组\n";
        cout << "32 创建群\n";
        cout << "---------------------------------------------------------------\n";
        cout << "[聊天与文件]\n";
        cout << "6 私聊                 7 私聊历史               8 群聊\n";
        cout << "9 群聊历史             10 发送私聊文件          11 下载文件\n";
        cout << "25 发送群聊文件\n";
        cout << "---------------------------------------------------------------\n";
        cout << "0 退出系统\n";
        cout << "===============================================================\n";

        int op;
        cin >> op;

        string data;
        string packet;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }

        if(op == 1) {
            chat::LoginReq req;
            int id;
            string pwd;

            cout << "id:";
            cin >> id;
            currentUserid = id;

            cout << "password:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            pwd = inputPassword();

            req.set_id(id);
            req.set_password(pwd);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::LOGIN_MSG, data);
        }
        else if(op == 2) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::AddFriendReq req;
            int toid;

            cout << "toid:";
            cin >> toid;

            req.set_toid(toid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::ADD_FRIEND_MSG, data);
        }
        else if(op == 3) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::QueryFriendReqReq req;
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::QUERY_FRIEND_REQ_MSG, data);
        }
        else if(op == 4) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::AcceptFriendReq req;
            int friendid;

            cout<<"friendid:";
            cin>>friendid;

            req.set_friendid(friendid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::ACCEPT_FRIEND_MSG, data);
        }
        else if(op == 0) {
            cout << "waiting file threads..." << endl;

            for(auto& t : fileThreads) {
                if(t.joinable()) t.join();
            }

            cout << "all file threads finished" << endl;
            running = false;

            if(recvThread.joinable()) recvThread.join();
            
            cout << "recv thread finished" << endl;

            if (ssl) {
                int ret = SSL_shutdown(ssl);
                if (ret == 0) ret = SSL_shutdown(ssl);
                if (ret < 0) {
                    int err = SSL_get_error(ssl, ret);
                    cout << "SSL_shutdown failed, error = " << err << endl;
                }

                SSL_free(ssl);
                ssl = nullptr;
            }

            if(sslCtx) {
                SSL_CTX_free(sslCtx);
                sslCtx = nullptr;
            }

            close(sockfd);

            cout << "client exit" << endl;
            break;
        }
        else if(op == 5) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::DeleteFriendReq req;
            int friendid;

            cout << "friendid:";
            cin >> friendid;

            req.set_friendid(friendid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::DELETE_FRIEND_MSG, data);
        }
        else if(op == 6) {
            if(currentUserid == -1){
                cout << "please login first" << endl;
                continue;
            }
            chat::OneChatReq req;
            int toid;
            cout << "toid:";
            cin >> toid;

            checkFriendFinished = false;
            checkFriendSuccess = false;
            checkFriend(ssl, toid);

            while(!checkFriendFinished) {
                usleep(10000);
            }
            if(!checkFriendSuccess) {
                cout << "无法私聊，即将退出私聊" << endl;
                continue;
            }
            cout << "互为好友，请继续输入消息" << endl;


            bool quitChat = false;
            cout << "msg(Enter发送，/quit退出):" << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            while (true) {
                string msg;
                getline(cin, msg);

                if (msg == "/quit") quitChat = true;
                if (quitChat) break;
                if (msg.empty()) {
                    continue;
                }

                req.set_toid(toid);
                req.set_msg(msg);
                req.SerializeToString(&data);
                packet = MessageCodec::encode(chat::ONE_CHAT_MSG, data);
                needSend = false;
                sendAll(ssl, packet.data(), packet.size());
            }
        }
        else if(op == 7) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::HistoryMsgReq req;
            int friendid;

            cout<<"friendid:";
            cin>>friendid;

            req.set_friendid(friendid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::HISTORY_MSG, data);
        }
        else if(op == 8) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::GroupChatReq req;
            int groupid;
            cout << "groupid:";
            cin >> groupid;

            checkGroupFinished = false;
            checkGroupSuccess = false;
            checkGroup(ssl, groupid);

            while(!checkGroupFinished) {
                usleep(10000);
            }
            if(!checkGroupSuccess) {
                cout << "无法群聊，即将退出群聊" << endl;
                continue;
            }
            cout << "可以群聊，请继续输入消息" << endl;

            bool quitChat = false;
            cout << "msg(Enter发送，/quit退出):" << endl;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            while (true) {
                string msg;
                getline(cin, msg);
                
                if (msg == "/quit") quitChat = true;
                if (quitChat) break;
                if (msg.empty()) {
                    continue;
                }

                req.set_groupid(groupid);
                req.set_msg(msg);
                req.SerializeToString(&data);
                packet = MessageCodec::encode(chat::GROUP_CHAT_MSG, data);
                needSend = false;
                sendAll(ssl, packet.data(), packet.size());
            }
        }
        else if(op == 9) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::GroupHistoryMsgReq req;
            int groupid;

            cout<<"groupid:";
            cin>>groupid;

            req.set_groupid(groupid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::GROUP_HISTORY_MSG, data);

        }
        else if(op == 10) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            int toid;
            string path;

            cout<<"toid:";
            cin>>toid;

            checkFriendFinished = false;
            checkFriendSuccess = false;
            checkFriend(ssl, toid);

            while(!checkFriendFinished) {
                usleep(10000);
            }
            if(!checkFriendSuccess) {
                cout << "不是好友，即将退出私聊文件" << endl;
                continue;
            }
            cout << "互为好友，请继续输入文件路径" << endl;

            cout<<"file path:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin,path);

            fileThreads.emplace_back(sendFile, ssl, toid, 0, path);
        }
        else if(op == 11) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            if (downloadActive) {
                cout << "当前正在下载文件，请等待下载完成" << endl;
                continue;
            }
            queryFile();
            string fileid;

            cout << "fileid:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, fileid);

            chat::DownloadFileReq req;
            req.set_fileid(fileid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::DOWNLOAD_FILE_MSG, data);
        }
        else if(op == 12) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::CancelAccountReq req;
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::CANCEL_ACCOUNT_MSG, data);
        }
        else if(op == 13) {
            chat::RegisterReq req;
            string email = "";

            cout << "email:";
            cin >> email;

            sendCode(ssl, email, 1);
            cout << "验证码已发送" << endl;

            string code;
            cout << "code:";
            cin >> code;

            verifyCodeFinished = false;
            verifyCodeSuccess = false;
            verifyCode(ssl,email,code,1);

            while(!verifyCodeFinished) {
                usleep(10000);
            }
            if(!verifyCodeSuccess) {
                cout << "验证码错误，注册结束" << endl;
                continue;
            }
            cout << "验证码正确，请继续输入用户名和密码" << endl;

            string name;
            string pwd;

            cout << "username:";
            cin >> name;

            cout << "password:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            pwd = inputPassword();

            req.set_name(name);
            req.set_password(pwd);
            req.set_email(email);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::REG_MSG, data);
        }
        else if(op == 14) {
            chat::ApplyGroupReq req;
            int groupid;

            cout<<"groupid:";
            cin>>groupid;

            req.set_groupid(groupid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::APPLY_GROUP_MSG, data);
        }
        else if(op == 15) {
            chat::QueryGroupReqReq req;
          
            req.set_msgid(chat::QUERY_GROUP_REQ_MSG);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::QUERY_GROUP_REQ_MSG, data);
        }
        else if(op == 16){
            chat::AcceptGroupReq req;
            int groupid;
            int targetid;

            cout << "groupid:";
            cin >> groupid;

            cout << "targetid:";
            cin >> targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);
            req.SerializeToString(&data);
            packet =MessageCodec::encode(chat::ACCEPT_GROUP_MSG, data);
        }
        else if(op == 17) {
            chat::LeaveGroupReq req;
            int groupid;

            cout << "groupid:";
            cin >> groupid;

            req.set_groupid(groupid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::LEAVE_GROUP_MSG, data);
        }
        else if(op == 18) {
            chat::TransferOwnerReq req;
            int newownerid;
            int groupid;

            cout << "new owner:";
            cin >> newownerid;

            cout << "groupid:";
            cin >> groupid;

            req.set_newownerid(newownerid);
            req.set_groupid(groupid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::TRANSFER_OWNER_MSG, data);
        }
        else if(op == 19) {
            chat::DissolveGroupReq req;
            int groupid;

            cout << "groupid:";
            cin >> groupid;

            req.set_groupid(groupid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::DISSOLVE_GROUP_MSG, data);
        }
        else if (op == 20) {
            chat::QueryGroupUserReq req;
            int groupid;

            cout<<"groupid:";
            cin>>groupid;

            req.set_groupid(groupid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::QUERY_GROUP_USER_MSG, data);
        }
        else if (op == 21) {
            chat::SetGroupAdminReq req;
            int groupid;
            int targetid;

            cout<<"groupid:";
            cin>>groupid;

            cout<<"targetid:";
            cin>>targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::SET_GROUP_ADMIN_MSG, data);
        }
        else if (op == 22) {
            chat::RemoveGroupAdminReq req;
            int groupid;
            int targetid;

            cout<<"groupid:";
            cin>>groupid;

            cout<<"targetid:";
            cin>>targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::REMOVE_GROUP_ADMIN_MSG, data);
        }
        else if (op == 23) {
            chat::RemoveGroupUserReq req;
            int groupid;
            int targetid;

            cout<<"groupid:";
            cin>>groupid;

            cout<<"targetid:";
            cin>>targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::REMOVE_GROUP_USER_MSG, data);
        }
        else if(op == 24) {
            chat::RefuseGroupReq req;
            int groupid;
            int targetid;

            cout << "groupid:";
            cin >> groupid;

            cout << "targetid:";
            cin >> targetid;

            req.set_groupid(groupid);
            req.set_targetid(targetid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::REFUSE_GROUP_MSG, data);
        }
        else if (op == 25) {
            int groupid;
            string filepath;

            cout << "groupid:";
            cin >> groupid;

            checkGroupFinished = false;
            checkGroupSuccess = false;
            checkGroup(ssl, groupid);

            while(!checkGroupFinished) {
                usleep(10000);
            }
            if(!checkGroupSuccess) {
                cout << "无法群聊，即将退出群聊文件" << endl;
                continue;
            }
            cout << "可以群聊，请继续输入文件路径" << endl;

            cout << "file path:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, filepath);

            fileThreads.emplace_back(sendFile, ssl, 0, groupid, filepath);
        }
        else if (op == 26) {
            if (currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            int blackid;

            cout << "blackid:";
            cin >> blackid;

            chat::BlacklistAddReq req;
            req.set_blackid(blackid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::BLACKLIST_ADD_MSG, data);
        }
        else if (op == 27) {
            if (currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            int blackid;

            cout << "blackid:";
            cin >> blackid;

            chat::BlacklistRemoveReq req;
            req.set_blackid(blackid);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::BLACKLIST_REMOVE_MSG, data);
        }
        else if(op == 28) {
            chat::CodeLoginReq req;
            string email = "";

            cout << "email:";
            cin >> email;

            sendCode(ssl, email, 2);
            cout<<"验证码已发送" << endl;

            string code;
            cout << "code:";
            cin >> code;

            verifyCodeFinished = false;
            verifyCodeSuccess = false;

            verifyCode(ssl, email, code, 2);

            while(!verifyCodeFinished) {
                usleep(10000);
            }
            if(!verifyCodeSuccess) {
                cout << "验证码错误，登录结束" << endl;
                continue;
            }

            cout << "验证码正确，正在登录..." << endl;

            req.set_email(email);
            req.set_code(code);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::CODE_LOGIN_MSG, data);
        }
        else if (op == 29)
        {
            string email;

            cout << "email:";
            cin >> email;

            sendCode(ssl, email, 3);
            cout << "验证码已发送" << endl;

            string code;

            cout << "code:";
            cin >> code;

            verifyCodeFinished = false;
            verifyCodeSuccess = false;

            verifyCode(ssl, email, code, 3);

            while (!verifyCodeFinished) {
                usleep(10000);
            }
            if (!verifyCodeSuccess) {
                cout << "验证码错误，密码找回结束" << endl;
                continue;
            }

            cout << "验证码正确，请输入新密码" << endl;

            string password;
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            password = inputPassword();

            chat::ResetPasswordReq req;

            cout << "email = [" << email << "]" << endl;
            cout << "password = [" << password << "]" << endl;
            cout << "password size = " << password.size() << endl;
            
            req.set_email(email);
            req.set_newpassword(password);
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::RESET_PASSWORD_MSG, data);
        }
        else if(op == 30) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::QueryFriendReq req;
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::QUERY_FRIEND_MSG, data);
        }
        else if(op == 31) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::QueryGroupReq req;
            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::QUERY_GROUP_MSG, data);
        }
        else if (op == 32) {
            if(currentUserid == -1) {
                cout << "please login first" << endl;
                continue;
            }
            chat::CreateGroupReq req;
            string groupname = "";
            string groupdesc = "";
            int friendid;
            cout << "groupname:";
            cin >> groupname;
            cout << "groupdesc:";
            cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            getline(cin, groupdesc);
            cout << "friendid:";
            cin >> friendid;

            req.set_groupname(groupname);
            req.set_groupdesc(groupdesc);
            req.set_friendid(friendid);

            req.SerializeToString(&data);
            packet = MessageCodec::encode(chat::CREATE_GROUP_MSG, data);
        }
        else continue;

        if (needSend && !packet.empty()) sendAll(ssl, packet.data(), packet.size());
    }

    return 0;
}
