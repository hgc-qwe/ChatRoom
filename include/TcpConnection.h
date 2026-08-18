#pragma once
#include <string>
#include <functional>
#include <memory>
#include <openssl/ssl.h>
#include <chrono>
#include <fstream>
#include "Channel.h"
#include "Buffer.h"

class EventLoop;

struct DownloadState {
    bool active = false;
    std::string fileid;
    std::string filename;
    std::string filepath;
    uint64_t filesize = 0;
    uint64_t offset = 0;
    std::ifstream file;
};
class TcpConnection : public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(int fd, EventLoop* loop, SSL_CTX* sslctx);
    ~TcpConnection();

    bool sendMessage(const std::string& msg);

    bool recvMessage();

    void sendBuffer();
    Buffer& getReadBuffer();

    void close();

    int getFd() const;

    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, Buffer&)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;

    void setConnectionCallback(ConnectionCallback cb);
    void setMessageCallback(MessageCallback cb);
    void setCloseCallback(CloseCallback cb);
    void connectEstablished();
    void removeChannel();

    void setUserId(int id);
    int getUserId();

    void updateLastActiveTime();
    bool isTimeout() const;

    EventLoop* getLoop() const;

    void sendPing();

    void handleClose();

    bool startDownload(const std::string& fileid, const std::string& filename, const std::string& filepath, uint64_t filesize);
    void sendDownloadChunk();
    void finishDownload();

    void setUploading(bool value);
    void setDownloading(bool value);
    bool isFileTransferring() const;
private:
    int fd;
    EventLoop* loop;
    int userid{-1};
    Buffer readBuffer;
    Buffer writeBuffer;
    std::shared_ptr<Channel> channel;
    SSL* ssl{nullptr};

    ConnectionCallback connectionCallback;
    MessageCallback messageCallback;
    CloseCallback closeCallback;

    DownloadState downloadState;

    bool tlsEstablished{false};
    bool tlsHandshake();

    bool uploading = false;
    bool downloading = false;

    std::chrono::steady_clock::time_point lastActiveTime;
};