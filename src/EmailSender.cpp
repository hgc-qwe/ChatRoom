#include <curl/curl.h>
#include <iostream>
#include <string>
#include <cstring>
#include <nlohmann/json.hpp>
#include <fstream>
#include "EmailSender.h"
#include "Logger.h"

using json = nlohmann::json;

EmailSender::EmailSender() {
    std::ifstream ifs("config.json");
    if (!ifs.is_open()) {
        throw std::runtime_error("open config.json failed");
    }
    json config;
    ifs >> config;

    smtpServer = config["smtp_server"];
    username = config["smtp_user"];
    password = config["smtp_password"];
}

bool EmailSender::sendCode(const std::string& email, const std::string& code) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string mail;
    mail += "From: <" + username + ">\r\n";
    mail += "To: <" + email + ">\r\n";
    mail += "Subject: ChatRoom 验证码\r\n";
    mail += "Content-Type: text/plain; charset=utf-8\r\n";
    mail += "\r\n";
    mail += "你的验证码是: " + code + "\r\n";
    mail += "   5分钟内有效\r\n";

    curl_easy_setopt(curl, CURLOPT_URL, smtpServer.c_str());
    curl_easy_setopt(curl, CURLOPT_PROXY, "");
    curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    std::string from = "<" + username + ">";
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str());

    struct curl_slist* recipients = nullptr;
    std::string to = "<" + email + ">";
    recipients = curl_slist_append(recipients, to.c_str());
    struct UploadStatus upload;
    upload.data = mail;
    upload.pos = 0;

    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payloadSource);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, mail.size());
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        LOG_ERROR("send email failed: {}", curl_easy_strerror(res));
    }
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

size_t EmailSender::payloadSource(void* ptr, size_t size, size_t nmemb, void* userdata) {
    UploadStatus* upload = static_cast<UploadStatus*>(userdata);
    size_t max = size * nmemb;
    size_t remain = upload->data.size() - upload->pos;
    size_t sendSize = remain < max ? remain : max;

    memcpy(ptr, upload->data.c_str() + upload->pos, sendSize);
    upload->pos += sendSize;

    return sendSize;
}