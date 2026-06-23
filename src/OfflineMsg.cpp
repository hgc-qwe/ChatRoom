#include <iostream>
#include "Mysql.h"
#include "OfflineMsg.h"

bool OfflineMsg::insert(int userid, std::string msg) {
    std::string sql = "insert into offlinemessage (userid, msg) values (" + std::to_string(userid) 
        + ",'" + msg + "');";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

bool OfflineMsg::remove(int userid) {
    std::string sql = "delete from offlinemessage where userid=" + std::to_string(userid) + ";";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

std::vector<std::string> OfflineMsg::query(int userid) {
    std::string sql = "select msg from offlinemessage where userid=" + std::to_string(userid) + ";";
    Mysql mysql;
    std::vector<std::string> msgs;
    if (!mysql.connect()) {
        return msgs;
    }
    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return msgs;
    }
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)) != nullptr) {
        msgs.push_back(row[0]);
    }
    mysql_free_result(res);
    return msgs;
}