#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "TcpServer.h"
#include "MessageCodec.h"
#include "Channel.h"
#include "Util.h"
#include "Logger.h"

TcpServer::TcpServer(const std::string& ip, int port) : ip(ip), port(port), threadPool(&loop, 3) {
    listenfd = -1;
}

TcpServer::~TcpServer() {
    if (listenfd != -1) close(listenfd);
    if (sslCtx != nullptr) {
        SSL_CTX_free(sslCtx);
        sslCtx = nullptr;
    }
}

bool TcpServer::init() {
    if (!initSSL()) return false;
    if (!createListenFd()) return false;

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &listen_addr.sin_addr) <= 0) {
        LOG_ERROR("invalid server ip: {}", ip);
        return false;
    }

    if (!bind()) {
        return false;
    }

    if (!listen()) {
        return false;
    }

    auto channel = std::make_shared<Channel>(&loop, listenfd);
    channel->setEvents(EPOLLIN);

    channel->setReadCallback([this]() {
        acceptConnection();
    });

    loop.addChannel(channel);

    return true;
}

void TcpServer::start() {
    threadPool.start();
    loop.loop();
}

bool TcpServer::createListenFd() {
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        LOG_ERROR("socket failed");
        return false;
    }

    int opt = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        LOG_ERROR("setsockopt SO_REUSEADDR failed");
        close(listenfd);
        listenfd = -1;
        return false;
    }
    if (setNonBlock(listenfd) == -1) {
        LOG_ERROR("setNonBlock failed");
        close(listenfd);
        listenfd = -1;
        return false;
    }
    return true;
}

bool TcpServer::bind() {
    if (::bind(listenfd, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        LOG_ERROR("bind failed");
        return false;
    }
    return true;
}

bool TcpServer::listen() {
    if (::listen(listenfd, 128) == -1) {
        LOG_ERROR("listen failed");
        return false;
    }
    return true;
}

void TcpServer::acceptConnection() {
    while(true) {
        int clientfd = ::accept4(listenfd, nullptr, nullptr, SOCK_NONBLOCK);
        if (clientfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            return;
        }

        EventLoop* ioLoop = threadPool.getNextLoop();
        ioLoop->runInLoop([this, clientfd, ioLoop]() {
            auto conn = std::make_shared<TcpConnection>(clientfd, ioLoop, sslCtx);
            ioLoop->addConnection(conn);
            {
                std::lock_guard<std::mutex> lock(connMutex);
                connections[clientfd] = conn;
            }

            conn->setMessageCallback([this](auto conn, Buffer& buffer) {
                int msgid;
                std::string data;

                while (MessageCodec::decode(buffer, msgid, data)) {
                    auto response = dispatcher.dispatch(conn, msgid, data);
                    if (!response.empty()) conn->sendMessage(response);
                } 
            });

            conn->setCloseCallback([this](std::shared_ptr<TcpConnection> conn) {
                removeConnection(conn);
            });
            
            conn->connectEstablished();
        });
    }
}

void TcpServer::closeConnection(int fd) {
    auto conn = connections[fd];
    connections.erase(fd);
    loop.getEpoll()->delFd(fd);
}

void TcpServer::removeConnection(std::shared_ptr<TcpConnection> conn) {
    int fd = conn->getFd();
    int userid = conn->getUserId();
    if (userid != -1) ChatService::instance()->clientClose(userid);
    {
        std::lock_guard<std::mutex> lock(connMutex);
        connections.erase(fd);
    }
    conn->getLoop()->removeConnection(fd);
    conn->close();
}

bool TcpServer::initSSL() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    const SSL_METHOD* method = TLS_server_method();
    sslCtx = SSL_CTX_new(method);
    if (sslCtx == nullptr) {
        LOG_ERROR("SSL_CTX_new failed");
        ERR_print_errors_fp(stderr);
        return false;
    }

    SSL_CTX_set_min_proto_version(sslCtx, TLS1_2_VERSION);

    if (SSL_CTX_use_certificate_file(sslCtx, "../cert/server.crt", SSL_FILETYPE_PEM) <= 0) {
        LOG_ERROR("load server certificate failed");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(sslCtx);
        sslCtx = nullptr;
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(sslCtx, "../cert/server.key", SSL_FILETYPE_PEM) <= 0) {
        LOG_ERROR("load server private key failed");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(sslCtx);
        sslCtx = nullptr;
        return false;
    }

    if (!SSL_CTX_check_private_key(sslCtx)) {
        LOG_ERROR("certificate and private key do not match");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(sslCtx);
        sslCtx = nullptr;
        return false;
    }

    LOG_INFO("TLS initialization success");
    return true;
}