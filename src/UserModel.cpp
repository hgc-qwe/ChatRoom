#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include "UserModel.h"
#include "Mysql.h"

bool UserModel::insert(User user) {
    std::string sql = "insert into user (id, name, password, state) values (" + std::to_string(user.getId()) 
        + ",'" + user.getName() + "','" + user.getPassword() + "','" + user.getState() + "');";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

User UserModel::query(int userid) {
    std::string sql = "select * from user where id=" + std::to_string(userid);
    Mysql mysql;
    User user;
    if (!mysql.connect()) {
        return user;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return User();
    }
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row != nullptr) {
        user.setId(atoi(row[0]));
        user.setName(row[1]);
        user.setPassword(row[2]);
        user.setState(row[3]);
    }
    mysql_free_result(res);
    return user;
}

bool UserModel::updateState(User user) {
    std::string sql = "update user set state='" + user.getState() + "' where id=" + std::to_string(user.getId()) + ";";
    Mysql mysql;
    if (! mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

bool UserModel::restState() {
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }

    std::string sql = "update user set state='offline' where state ='online';";
    return mysql.update(sql);
}