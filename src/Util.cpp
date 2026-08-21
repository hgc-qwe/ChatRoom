#include "Util.h"
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>

int setNonBlock(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        perror("fcntl get");
        return -1;
    }
    flag |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flag) == -1) {
        perror("fcntl set");
        return -1;
    }

    return 0;
}

std::string getCurrentTime()
{
    char buf[32];
    time_t now = time(nullptr);

    strftime(buf, sizeof(buf), "%F %T", localtime(&now));

    return buf;
}

std::string inputPassword() {
    termios oldt{};
    termios newt{};
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::string password;
    std::getline(std::cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << std::endl;
    return password;
}

namespace {

constexpr const char* HASH_PREFIX = "pbkdf2_sha256";
constexpr int HASH_ITERATIONS = 210000;
constexpr size_t SALT_LEN = 16;
constexpr size_t HASH_LEN = 32;

std::string base64Encode(const unsigned char* data, size_t len) {
    std::vector<char> buf(4 * ((len + 2) / 3) + 1);
    int n = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(buf.data()), data, static_cast<int>(len));
    if (n < 0) return std::string();
    return std::string(buf.data(), n);
}

bool base64Decode(const std::string& str, std::vector<unsigned char>& out) {
    out.resize(3 * str.size() / 4 + 1);
    int n = EVP_DecodeBlock(out.data(), reinterpret_cast<const unsigned char*>(str.data()), static_cast<int>(str.size()));
    if (n < 0) return false;

    size_t padding = 0;
    if (!str.empty() && str[str.size() - 1] == '=') padding++;
    if (str.size() > 1 && str[str.size() - 2] == '=') padding++;
    out.resize(static_cast<size_t>(n) - padding);
    return true;
}

bool pbkdf2(const std::string& password, const unsigned char* salt, size_t saltLen, int iterations, unsigned char* out, size_t outLen) {
    return PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()), salt, static_cast<int>(saltLen), iterations, EVP_sha256(), static_cast<int>(outLen), out) == 1;
}

}

std::string hashPassword(const std::string& password) {
    unsigned char salt[SALT_LEN];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        return std::string();
    }

    unsigned char hash[HASH_LEN];
    if (!pbkdf2(password, salt, sizeof(salt), HASH_ITERATIONS, hash, sizeof(hash))) {
        return std::string();
    }

    std::string encoded = std::string(HASH_PREFIX) + "$" + std::to_string(HASH_ITERATIONS) + "$" + base64Encode(salt, sizeof(salt)) + "$" + base64Encode(hash, sizeof(hash));

    OPENSSL_cleanse(hash, sizeof(hash));
    return encoded;
}

bool isPasswordHashed(const std::string& stored) {
    if (stored.compare(0, strlen(HASH_PREFIX), HASH_PREFIX) != 0) return false;
    return std::count(stored.begin(), stored.end(), '$') == 3;
}

bool verifyPassword(const std::string& password, const std::string& stored) {
    if (!isPasswordHashed(stored)) return false;

    size_t p1 = stored.find('$');
    size_t p2 = stored.find('$', p1 + 1);
    size_t p3 = stored.find('$', p2 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos || p3 == std::string::npos) return false;

    int iterations = 0;
    try {
        iterations = std::stoi(stored.substr(p1 + 1, p2 - p1 - 1));
    } catch (...) {
        return false;
    }
    if (iterations <= 0) return false;

    std::vector<unsigned char> salt;
    std::vector<unsigned char> expected;
    if (!base64Decode(stored.substr(p2 + 1, p3 - p2 - 1), salt)) return false;
    if (!base64Decode(stored.substr(p3 + 1), expected)) return false;
    if (salt.empty() || expected.empty()) return false;

    std::vector<unsigned char> actual(expected.size());
    if (!pbkdf2(password, salt.data(), salt.size(), iterations, actual.data(), actual.size())) {
        return false;
    }

    bool match = CRYPTO_memcmp(actual.data(), expected.data(), actual.size()) == 0;
    OPENSSL_cleanse(actual.data(), actual.size());
    return match;
}