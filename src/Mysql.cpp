#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include <vector>
#include "Mysql.h"
#include "Logger.h"

Mysql::Mysql() {
    conn = mysql_init(nullptr);
    if (conn == nullptr) LOG_ERROR("mysql_init failed");
}

bool Mysql::connect() {
    if (conn == nullptr) {
        LOG_ERROR("mysql connect failed: handle not initialized");
        return false;
    }

    MYSQL* ret = mysql_real_connect(conn, "127.0.0.1", "root", "123456", "chatroom", 3306, nullptr, 0);
    if (ret == nullptr) {
        LOG_ERROR("mysql connect failed: {}", mysql_error(conn));
        return false;
    }

    mysql_set_character_set(conn, "utf8mb4");
    return true;
}

bool Mysql::update(const std::string& sql) {
    if (conn == nullptr) return false;
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("mysql update failed, sql={}, error={}", sql, mysql_error(conn));
        return false;
    }
    return true;
}

MYSQL_RES* Mysql::query(const std::string& sql) {
    if (conn == nullptr) return nullptr;
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("mysql query failed, sql={}, error={}", sql, mysql_error(conn));
        return nullptr;
    }
    return mysql_store_result(conn);
}

MYSQL* Mysql::getcon() {
    return conn;
}

bool Mysql::ping() {
    if (conn == nullptr) return false;
    return mysql_ping(conn) == 0;
}

std::string Mysql::escape(const std::string& str) {
    if (conn == nullptr) return std::string();

    std::vector<char> buf(str.size() * 2 + 1);
    unsigned long len = mysql_real_escape_string(conn, buf.data(), str.data(), str.size());
    return std::string(buf.data(), len);
}

Mysql::~Mysql() {
    if (conn != nullptr) {
        mysql_close(conn);
        conn = nullptr;
    }
}