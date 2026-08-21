#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include "UserModel.h"
#include "Mysql.h"
#include "MysqlPool.h"

bool UserModel::insert(User& user) {
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }
    std::string sql = "insert into user (name, password, state, email) values ('" + mysql.escape(user.getName()) + "','" + mysql.escape(user.getPassword()) + "','" + mysql.escape(user.getState()) + "','" + mysql.escape(user.getEmail()) + "');";
    if (mysql.update(sql)) {
        user.setId(mysql_insert_id(mysql.getcon()));
        return true;
    }
    return false;
}

User UserModel::query(int userid) {
    std::string sql = "select * from user where id=" + std::to_string(userid);
    auto mysql = MysqlPool::instance()->acquire();
    User user;
    if (!mysql) {
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
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }
    return mysql.update(sql);
}

bool UserModel::restState() {
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }

    std::string sql = "update user set state='offline' where state ='online';";
    return mysql.update(sql);
}

bool UserModel::remove(int userid) {
    std::string sql = "delete from user where id=" + std::to_string(userid) + ";";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }
    return mysql.update(sql);
}

User UserModel::queryByEmail(const std::string& email) {
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) return User();

    std::string sql = "select id, name from user where email='" + mysql.escape(email) + "';";
    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return User();

    User user;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        user.setId(atoi(row[0]));
        user.setName(row[1] ? row[1] : "");
    }
    
    mysql_free_result(res);
    return user;
}

bool UserModel::updatePassword(int userid, const std::string& password) {
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }
    std::string sql = "update user set password='" + mysql.escape(password) + "' where id=" + std::to_string(userid) + ";";
    return mysql.update(sql);
}

bool UserModel::isExist(int id) {
    std::string sql ="select id from user where id = " + std::to_string(id) + ";";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(res);

    mysql_free_result(res);
    return row != nullptr;
}