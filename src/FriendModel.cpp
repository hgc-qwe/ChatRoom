#include <iostream>
#include "UserModel.h"
#include "Mysql.h"
#include "MysqlPool.h"
#include "FriendModel.h"

bool FriendModel::insert(int userid, int friendid) {
    std::string sql1 = "insert into friend (userid, friendid) values (" + std::to_string(userid) 
        + "," + std::to_string(friendid) + ");";
    std::string sql2 = "insert into friend (userid, friendid) values (" + std::to_string(friendid) 
        + "," + std::to_string(userid) + ");";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }
    if (!mysql.update(sql1)) return false;
    if (!mysql.update(sql2)) return false;
    return true;
}

std::vector<User> FriendModel::query(int userid) {
    std::string sql ="select a.id,a.name,a.state from user a inner join friend b on a.id=b.friendid where b.userid="+ std::to_string(userid);
    auto mysql = MysqlPool::instance()->acquire();
    std::vector<User> friends;
    if (!mysql) {
        return friends;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return friends;
    }
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)) != nullptr) {
        User user;
        user.setId(atoi(row[0]));
        user.setName(row[1]);
        user.setState(row[2]);

        friends.push_back(user);
    }
    mysql_free_result(res);
    return friends;
}

bool FriendModel::isFriend(int fromid, int toid) {
    std::string sql = "select 1 from friend where userid=" + std::to_string(fromid)
        + " and friendid=" + std::to_string(toid) + " limit 1;";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return false;
    }

    bool exist = mysql_fetch_row(res) != nullptr;
    mysql_free_result(res);
    return exist;
}

bool FriendModel::remove(int userid, int friendid) {
    std::string sql ="delete from friend where userid="+ std::to_string(userid) + " and friendid=" + std::to_string(friendid) + ";";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }

    return mysql.update(sql);
}

bool FriendModel::removeAll(int userid) {
    std::string sql ="delete from friend where userid="+ std::to_string(userid) + " or friendid=" + std::to_string(userid) + ";";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }

    return mysql.update(sql);
}