#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include "Mysql.h"
#include "Logger.h"

Mysql::Mysql() {
    conn = mysql_init(nullptr);
    if (conn == nullptr) LOG_ERROR("mysql_init failed");
}

bool Mysql::connect() {
    conn = mysql_real_connect(conn, "127.0.0.1", "root", "123456", "chatroom", 3306, nullptr, 0);
    if (conn == nullptr) {
        LOG_ERROR("mysql connect failed: {}", mysql_error(conn));
        return false;
    }
    return true;
}

bool Mysql::update(const std::string& sql) {
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("mysql update failed, sql={}, error={}", sql, mysql_error(conn));
        return false;
    }
    return true;
}

MYSQL_RES* Mysql::query(const std::string& sql) {
    if (mysql_query(conn, sql.c_str())) {
        LOG_ERROR("mysql query failed, sql={}, error={}", sql, mysql_error(conn));
        return nullptr;
    }
    return mysql_store_result(conn);
}

MYSQL* Mysql::getcon() {
    return conn;
}

Mysql::~Mysql() {
    if (conn != nullptr) {
        mysql_close(conn);
    }
}