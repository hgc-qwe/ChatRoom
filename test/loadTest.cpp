#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

struct Client {
    int fd = -1;
    SSL* ssl = nullptr;
};

std::atomic<int> connectSuccess{0};
std::atomic<int> tlsSuccess{0};
std::atomic<int> connectFailed{0};
std::atomic<int> tlsFailed{0};

void testClient(const std::string& ip, int port, int keepSeconds)
{
    Client client;

    // 1. 创建 socket
    client.fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client.fd == -1) {
        connectFailed++;
        return;
    }

    // 2. 设置服务器地址
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &server.sin_addr) <= 0) {
        close(client.fd);
        connectFailed++;
        return;
    }

    // 3. TCP connect
    if (connect(
        client.fd,
        reinterpret_cast<sockaddr*>(&server),
        sizeof(server)
    ) < 0) {
        close(client.fd);
        connectFailed++;
        return;
    }

    connectSuccess++;

    // 4. 创建 TLS context
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());

    if (ctx == nullptr) {
        close(client.fd);
        tlsFailed++;
        return;
    }

    // 测试环境使用自签名证书，所以不验证服务器证书
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    // 5. 创建 SSL
    client.ssl = SSL_new(ctx);

    if (client.ssl == nullptr) {
        SSL_CTX_free(ctx);
        close(client.fd);
        tlsFailed++;
        return;
    }

    SSL_set_fd(client.ssl, client.fd);

    // 6. TLS handshake
    if (SSL_connect(client.ssl) <= 0) {
        tlsFailed++;

        SSL_free(client.ssl);
        SSL_CTX_free(ctx);
        close(client.fd);

        return;
    }

    tlsSuccess++;

    // 7. 保持连接
    std::this_thread::sleep_for(
        std::chrono::seconds(keepSeconds)
    );

    // 8. 关闭 TLS
    SSL_shutdown(client.ssl);
    SSL_free(client.ssl);
    SSL_CTX_free(ctx);

    close(client.fd);
}

int main(int argc, char* argv[])
{
    // 默认参数
    std::string ip = "127.0.0.1";
    int port = 8000;
    int clientCount = 100;
    int keepSeconds = 60;

    /*
        使用方式：

        ./loadTest

        ./loadTest 127.0.0.1 8000 100 60

        参数：
        argv[1] = IP
        argv[2] = port
        argv[3] = 客户端数量
        argv[4] = 保持连接时间（秒）
    */

    if (argc >= 2) {
        ip = argv[1];
    }

    if (argc >= 3) {
        try {
            port = std::stoi(argv[2]);
        }
        catch (...) {
            std::cerr << "invalid port: " << argv[2] << std::endl;
            return -1;
        }
    }

    if (argc >= 4) {
        try {
            clientCount = std::stoi(argv[3]);
        }
        catch (...) {
            std::cerr << "invalid client count: "
                      << argv[3] << std::endl;
            return -1;
        }
    }

    if (argc >= 5) {
        try {
            keepSeconds = std::stoi(argv[4]);
        }
        catch (...) {
            std::cerr << "invalid keep seconds: "
                      << argv[4] << std::endl;
            return -1;
        }
    }

    if (argc >= 6) {
        std::cerr
            << "usage: ./loadTest [ip] [port] [clientCount] [keepSeconds]"
            << std::endl;
        return -1;
    }

    std::cout << "========== Load Test ==========" << std::endl;
    std::cout << "server   : " << ip << ":" << port << std::endl;
    std::cout << "clients  : " << clientCount << std::endl;
    std::cout << "keep     : " << keepSeconds << " seconds" << std::endl;
    std::cout << "================================" << std::endl;

    // OpenSSL 初始化
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(clientCount);

    // 创建客户端
    for (int i = 0; i < clientCount; ++i) {

        threads.emplace_back(
            testClient,
            ip,
            port,
            keepSeconds
        );
    }

    // 等待所有客户端结束
    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::steady_clock::now();

    double elapsed =
        std::chrono::duration<double>(end - start).count();

    std::cout << std::endl;
    std::cout << "========== Result ==========" << std::endl;
    std::cout << "total clients : " << clientCount << std::endl;
    std::cout << "TCP success   : " << connectSuccess << std::endl;
    std::cout << "TCP failed    : " << connectFailed << std::endl;
    std::cout << "TLS success   : " << tlsSuccess << std::endl;
    std::cout << "TLS failed    : " << tlsFailed << std::endl;
    std::cout << "elapsed       : " << elapsed << " seconds" << std::endl;
    std::cout << "============================" << std::endl;

    EVP_cleanup();

    return 0;
}