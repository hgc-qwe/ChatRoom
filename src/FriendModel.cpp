#include <iostream>
#include "UserModel.h"
#include "Mysql.h"
#include "FriendModel.h"

bool FriendModel::insert(int userid, int friendid) {
    std::string sql = "insert into friend (userid, friendid) values (" + std::to_string(userid) 
        + "," + std::to_string(friendid) + ");";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

std::vector<User> FriendModel::query(int userid) {
    std::string sql ="select a.id,a.name,a.state from user a inner join friend b on a.id=b.friendid where b.userid="+ std::to_string(userid);
    Mysql mysql;
    std::vector<User> friends;
    if (!mysql.connect()) {
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