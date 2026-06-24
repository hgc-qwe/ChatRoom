#include <iostream>
#include <string>
#include "Mysql.h"
#include "GroupModel.h"

bool GroupModel::createGroup(Group &group) {
    std::string sql = "insert into allgroup (groupname, groupdesc) values ('" + group.getName() + "','" + group.getDesc() + "');";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    if (mysql.update(sql)) {
        group.setId(mysql_insert_id(mysql.getcon()));
        return true;
    }
    return false;
}

bool GroupModel::addGroup(int userid, int groupid, std::string role) {
    std::string sql = "insert into groupuser (userid, groupid, grouprole) values (" + std::to_string(userid) + "," + std::to_string(groupid) + ",'" + role + "');";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

std::vector<Group> GroupModel::queryGroups(int userid) {
    std::string sql = "select g.id, g.groupname, g.groupdesc from allgroup g inner join groupuser gu on g.id = gu.groupid where gu.userid =" + std::to_string(userid) + ";";
    Mysql mysql;
    std::vector<Group> groups;
    if (!mysql.connect()) {
        return groups;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return groups;
    }
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)) != nullptr) {
        Group group(row[1], row[2]);
        group.setId(atoi(row[0]));
        groups.push_back(group);
    }
    mysql_free_result(res);
    return groups;
}

std::vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    std::string sql = "select u.id from user u inner join groupuser gu on u.id = gu.userid where gu.groupid =" + std::to_string(groupid) + " and gu.userid !=" + std::to_string(userid) + ";";
    Mysql mysql;
    std::vector<int> usersid;
    if (!mysql.connect()) {
        return usersid;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return usersid;
    }
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)) != nullptr) {
        usersid.push_back(atoi(row[0]));
    }
    mysql_free_result(res);
    return usersid;
}