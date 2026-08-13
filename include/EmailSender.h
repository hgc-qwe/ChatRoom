#pragma once
#include <string>

class EmailSender {
private:
    std::string smtpServer;
    std::string username;
    std::string password;
public:
    EmailSender();

    bool sendCode(const std::string& email, const std::string& code);

    static size_t payloadSource(void* ptr, size_t size, size_t nmemb, void* userdata);
};

struct UploadStatus {
    std::string data;
    size_t pos;
};