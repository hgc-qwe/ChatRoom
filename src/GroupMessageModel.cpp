#include <vector>
#include <string>
#include "Mysql.h"
#include "GroupMessage.h"
#include "GroupMessageModel.h"

bool GroupMessageModel::insert(int groupid, int userid, std::string msg) {
    std::string sql = "insert into group_message (groupid, userid, msg) values (" 
         + std::to_string(groupid) + "," + std::to_string(userid) + ",'" + msg + "');";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

std::vector<GroupMessage> GroupMessageModel::query(int groupid) {
    std::string sql = "select * from group_message where groupid=" + std::to_string(groupid) + " order by sendtime;";
    Mysql mysql;
    std::vector<GroupMessage> msgs;
    if (!mysql.connect()) return msgs;
    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return msgs;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        GroupMessage msg;
        msg.setId(atoi(row[0]));
        msg.setGroupid(atoi(row[1]));
        msg.setUserid(atoi(row[2]));
        msg.setMsg(row[3]);
        msg.setTime(row[4]);
        msgs.push_back(msg);
    }
    mysql_free_result(res);
    return msgs;
}