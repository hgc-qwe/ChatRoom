#include <mysql/mysql.h>
#include <iostream>
#include <string>
#include "Mysql.h"

Mysql::Mysql() {
    conn = mysql_init(nullptr);
}

bool Mysql::connect() {
    conn = mysql_real_connect(conn, "127.0.0.1", "root", "123456", "itcast", 3306, nullptr, 0);
    if (conn == nullptr) {
        std::cout << "connect failed" << std::endl;
        return false;
    }
    return true;
}

bool Mysql::update(const std::string& sql) {
    if (mysql_query(conn, sql.c_str())) {
        return false;
    }
    return true;
}

MYSQL_RES* Mysql::query(const std::string& sql) {
    if (mysql_query(conn, sql.c_str())) {
        return nullptr;
    }
    return mysql_store_result(conn);
}

Mysql::~Mysql() {
    if (conn != nullptr) {
        mysql_close(conn);
    }
}