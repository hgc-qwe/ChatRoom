#include <sys/socket.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <iostream>
#include <cerrno>
#include <algorithm>
#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"
#include "EventLoop.h"
#include "MessageCodec.h"
#include "chat.pb.h"

TcpConnection::TcpConnection(int fd, EventLoop* loop, SSL_CTX* sslCtx) {
    this->fd = fd;
    this->loop = loop;

    lastActiveTime = std::chrono::steady_clock::now();

    ssl = SSL_new(sslCtx);
    if (ssl == nullptr) {
        LOG_ERROR("SSL_new failed");
        ERR_print_errors_fp(stderr);
        return;
    }
    SSL_set_fd(ssl, fd);
    SSL_set_accept_state(ssl);

    channel = std::make_shared<Channel>(loop, fd);
    channel->setEvents(EPOLLIN);
    channel->setReadCallback([this]() {
        recvMessage();
    });
    channel->setWriteCallback([this]() {
        sendBuffer();
    });
    channel->setCloseCallback([this]() {
        if (closeCallback) closeCallback(shared_from_this());
    });
    loop->addChannel(channel);
    tlsHandshake();
}

TcpConnection::~TcpConnection() {
    close();
    if (ssl != nullptr) {
        SSL_free(ssl);
        ssl = nullptr;
    }
}

bool TcpConnection::sendMessage(const std::string& msg) {
    auto self = shared_from_this();
    loop->runInLoop([self, msg]() {
        self->writeBuffer.append(msg);
        self->channel->enableWriting();
    });
    return true;
}

bool TcpConnection::recvMessage() {
    char buf[BUFSIZ];
    if (!tlsEstablished) {
        tlsHandshake();
        if (!tlsEstablished) return true;
    }
    while (true) {
        int count = SSL_read(ssl, buf, sizeof(buf));
        if (count > 0) {
            updateLastActiveTime();
            readBuffer.append(buf, count);
        } else {
            int error = SSL_get_error(ssl, count);
            if (error == SSL_ERROR_WANT_READ) break;
            if (error == SSL_ERROR_WANT_WRITE) {
                channel->enableWriting();
                break;
            }
            if (error == SSL_ERROR_ZERO_RETURN) {
                if (closeCallback) closeCallback(shared_from_this());
                return false;
            }
            if (error == SSL_ERROR_SYSCALL) {
                LOG_INFO("TCP connection close fd = {}", fd);
                if (closeCallback) closeCallback(shared_from_this());
                return false;
            }

            LOG_ERROR("SSL_read failed fd = {}", fd);
            ERR_print_errors_fp(stderr);

            if (closeCallback) closeCallback(shared_from_this());
            return false;
        }
    }

    if (messageCallback && readBuffer.readableBytes() > 0) {
        messageCallback(shared_from_this(), readBuffer);
    }
    return true;
}

void TcpConnection::close() {
    if (fd != -1) {
        if (ssl != nullptr) SSL_shutdown(ssl);
        loop->removeChannel(fd);
        ::close(fd);
        fd = -1;
    }
}

int TcpConnection::getFd() const {
    return fd;
}

void TcpConnection::sendBuffer() {
    if (!tlsEstablished) {
        tlsHandshake();
        if (!tlsEstablished) return;
    }

    while (writeBuffer.readableBytes() > 0) {
        int n = SSL_write(ssl, writeBuffer.beginRead(), static_cast<int>(writeBuffer.readableBytes()));
        if (n > 0) {
            writeBuffer.retrieve(n);
            LOG_INFO("SSL_write fd={}, write={}, remian={}", fd, n, writeBuffer.readableBytes());
            continue;
        } 
        int error = SSL_get_error(ssl, n);
        if (error == SSL_ERROR_WANT_WRITE) {
            channel->enableWriting();
            return;
        }
        if (error == SSL_ERROR_WANT_READ) {
            channel->enableReading();
            return;
        }

        LOG_ERROR("SSL_write failed fd = {}， ret = {}, ssl_error = {}, errno = {}", fd, n, error, errno);
        ERR_print_errors_fp(stderr);
        close();
        return;
    }
    if (downloadState.active) {
        sendDownloadChunk();
        if (writeBuffer.readableBytes() > 0) {
            channel->enableWriting();
            return;
        }
        if (!downloadState.active) {
            channel->disableWriting();
            return;
        }
    }
    if (writeBuffer.readableBytes() == 0 && !downloadState.active) channel->disableWriting();
}

Buffer& TcpConnection::getReadBuffer() {
    return readBuffer;
}

void TcpConnection::setConnectionCallback(ConnectionCallback cb) {
    connectionCallback = cb;
}

void TcpConnection::setMessageCallback(MessageCallback cb) {
    messageCallback = cb;
}

void TcpConnection::setCloseCallback(CloseCallback cb) {
    closeCallback = cb;
}

void TcpConnection::connectEstablished() {
    if (connectionCallback) {
        connectionCallback(shared_from_this());
    }
}

void TcpConnection::removeChannel() {
    loop->removeChannel(fd);
}

void TcpConnection::setUserId(int id) {
    userid = id;
}

int TcpConnection::getUserId() {
    return userid;
}

bool TcpConnection::tlsHandshake() {
    int ret = SSL_accept(ssl);
    if (ret == 1) {
        tlsEstablished = true;
        LOG_INFO("TLS handshake success fd = {}", fd);
        return true;
    }

    int error = SSL_get_error(ssl, ret);
    if (error == SSL_ERROR_WANT_READ) {
        channel->enableReading();
        return false;
    }
    if (error == SSL_ERROR_WANT_WRITE) {
        channel->enableWriting();
        return false;
    }
    LOG_ERROR("TLS hanshake failed fd = {}", fd);
    ERR_print_errors_fp(stderr);
    close();
    return false;
}

void TcpConnection::updateLastActiveTime() {
    lastActiveTime = std::chrono::steady_clock::now();
    std::cout << "[Heartbeat] update pong fd = " << fd << std::endl;
}

bool TcpConnection::isTimeout() const {
    if (isFileTransferring()) return false;
    auto now = std::chrono::steady_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now - lastActiveTime).count();
    return seconds > 30;
}

EventLoop* TcpConnection::getLoop() const {
    return loop;
}

void TcpConnection::sendPing() {
    std::string packet = MessageCodec::encode(chat::PING_MSG, "");
    sendMessage(packet);
}

void TcpConnection::handleClose() {
    auto self = shared_from_this();
    if (closeCallback) closeCallback(self);
}

bool TcpConnection::startDownload(const std::string& fileid, const std::string& filename, const std::string& filepath, uint64_t filesize) {
    if (downloadState.active) {
        LOG_ERROR("connection already has a download");
        return false;
    }
    downloadState.fileid = fileid;
    downloadState.filename = filename;
    downloadState.filepath = filepath;
    downloadState.filesize = filesize;
    downloadState.offset = 0;

    downloadState.file.open(filepath, std::ios::binary);

    if (!downloadState.file.is_open()) {
        LOG_ERROR("open download file failed: {}", filepath);
        return false;
    }

    downloadState.active = true;
    setDownloading(true);

    chat::DownloadStart start;
    start.set_fileid(fileid);
    start.set_filename(filename);
    start.set_filesize(filesize);

    std::string data;
    start.SerializeToString(&data);

    writeBuffer.append(MessageCodec::encode(chat::DOWNLOAD_START_MSG, data));
    channel->enableWriting();
    return true;
}

void TcpConnection::sendDownloadChunk() {
    if (!downloadState.active) return;

    constexpr size_t CHUNK_SIZE = 64 * 1024;
    char buf[CHUNK_SIZE];

    downloadState.file.read(buf, CHUNK_SIZE);
    std::streamsize n = downloadState.file.gcount();
    if (n <= 0) {
        finishDownload();
        return;
    }
    chat::DownloadChunk chunk;
    chunk.set_fileid(downloadState.fileid);
    chunk.set_data(buf, n);
    chunk.set_offset(downloadState.offset);

    std::string body;
    chunk.SerializeToString(&body);
    std::string packet = MessageCodec::encode(chat::DOWNLOAD_CHUNK_MSG, body);

    writeBuffer.append(packet);
    downloadState.offset += n;
}

void TcpConnection::finishDownload() {
    if (!downloadState.active) return;

    chat::DownloadEnd end;
    end.set_fileid(downloadState.fileid);

    std::string data;
    end.SerializeToString(&data);
    writeBuffer.append(MessageCodec::encode(chat::DOWNLOAD_END_MSG, data));

    downloadState.file.close();
    downloadState.active = false;
    setDownloading(false);
    downloadState.fileid.clear();
    downloadState.filename.clear();
    downloadState.filepath.clear();
    downloadState.filesize = 0;
    downloadState.offset = 0;
}

void TcpConnection::setUploading(bool value) {
    uploading = value;
}
    
void TcpConnection::setDownloading(bool value) {
    downloading = value;
}
    
bool TcpConnection::isFileTransferring() const {
    return uploading || downloading;
}