#include <vector>
#include <string>
#include <cstring>
#include "Mysql.h"
#include "Message.h"
#include "MessageModel.h"

bool MessageModel::insert(int fromid, int toid, std::string msg, std::string fromname) {
    std::string sql = "insert into message (fromid, toid, msg, fromname) values (" 
         + std::to_string(fromid) + "," + std::to_string(toid) + ",'" + msg + "','" + fromname + "');";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

std::vector<Message> MessageModel::queryOffline(int userid) {
    std::string sql = "select * from message where toid=" + std::to_string(userid) + " and status=0;";
    Mysql mysql;
    std::vector<Message> msgs;
    if (!mysql.connect()) return msgs;
    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return msgs;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        Message msg;
        msg.setId(atoi(row[0]));
        msg.setFromid(atoi(row[1]));
        msg.setToid(atoi(row[2]));
        msg.setMsg(row[3]);
        msg.setTime(row[4]);
        msg.setFromname(row[5]);
        msgs.push_back(msg);
    }
    mysql_free_result(res);
    return msgs;
}

bool MessageModel::updateStatus(int userid) {
    std::string sql = "update message set status=1 where toid=" + std::to_string(userid) + " and status=0;";
    Mysql mysql;
    if (!mysql.connect()) return false;
    return mysql.update(sql);
}

std::vector<Message> MessageModel::queryHistory(int userid, int friendid) {
    std::string sql = "select * from message where (fromid=" + std::to_string(userid) + " and toid=" + std::to_string(friendid) + ") or (fromid=" + std::to_string(friendid) + " and toid=" + std::to_string(userid) + ") order by sendtime;";
    Mysql mysql;
    std::vector<Message> msgs;
    if (!mysql.connect()) return msgs;
    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return msgs;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        Message msg;
        msg.setId(atoi(row[0]));
        msg.setFromid(atoi(row[1]));
        msg.setToid(atoi(row[2]));
        msg.setMsg(row[3]);
        msg.setTime(row[4]);
        msg.setFromname(row[5]);
        msgs.push_back(msg);
    }
    mysql_free_result(res);
    return msgs;
}