#include <vector>
#include <iostream>
#include "GroupReqModel.h"
#include "Mysql.h"
#include "GroupRequest.h"

bool GroupReqModel::insert(int groupid, int userid) {
    std::string sql = "insert into group_request(groupid, userid) values(" + std::to_string(groupid) + "," + std::to_string(userid) + ");";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    
    return mysql.update(sql);
}

std::vector<GroupRequest> GroupReqModel::query(int groupid) {
    std::string sql = "select groupid, userid from group_request where groupid=" + std::to_string(groupid) + " and status=0;";
    Mysql mysql;
    std::vector<GroupRequest> requests;
    if (!mysql.connect()) return requests;
    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return requests;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        int groupid = atoi(row[0]);
        int userid = atoi(row[1]);
        requests.emplace_back(groupid, userid);
    }
    mysql_free_result(res);
    return requests;
}

bool GroupReqModel::update(int groupid, int userid, int status) {
    std::string sql = "update group_request set status=" + std::to_string(status) + " where groupid=" + std::to_string(groupid) + " and userid=" + std::to_string(userid) + ";";
    Mysql mysql;
    if (! mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

bool GroupReqModel::remove(int groupid, int userid) {
    std::string sql ="delete from group_request where groupid="+ std::to_string(groupid) + " and userid=" + std::to_string(userid) + ";";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }

    return mysql.update(sql);
}

bool GroupReqModel::removeAll(int userid) {
    std::string sql ="delete from group_request where userid="+ std::to_string(userid) + ";";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }

    return mysql.update(sql);
}

bool GroupReqModel::isApplied(int userid, int groupid) {
    std::string sql = "select * from group_request where groupid=" + std::to_string(groupid) + " and userid=" + std::to_string(userid) + " and status=0;";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res != nullptr) {
        bool exist = mysql_num_rows(res) > 0;
        mysql_free_result(res);
        return exist;
    }
    return false;
}