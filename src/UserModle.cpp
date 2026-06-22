#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include "UserModle.h"
#include "Mysql.h"

bool UserModle::insert(User user) {
    std::string sql = "insert into user (id, name, password, state) values (" + std::to_string(user.getId()) 
        + ",'" + user.getName() + "','" + user.getPassword() + "','" + user.getSate() + "');";
    Mysql mysql;
    if (mysql.connect()) {
        return mysql.update(sql);
    }
}

User UserModle::query(int userid) {
    std::string sql = "select * from user where id=" + std::to_string(userid);
    Mysql mysql;
    MYSQL_RES* res = mysql.query(sql);
    MYSQL_ROW row = mysql_fetch_row(res);

    User user;
    if (row != nullptr) {
        user.setId(atoi(row[0]));
        user.setName(row[1]);
        user.setPassword(row[2]);
        user.setSate(row[3]);
    }
    mysql_free_result(res);
    return user;
}

bool UserModle::updateState(User user) {
    std::string sql = "update user set state='" + user.getSate() + "' where id=" + std::to_string(user.getId()) + ";";
    Mysql mysql;
    if (mysql.connect()) {
        return mysql.update(sql);
    }
}

bool UserModle::restState() {
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }

    std::string sql = "update user set state='offline' where state ='online';";
    return mysql.update(sql);
}