#include <iostream>
#include <string>
#include "Mysql.h"
#include "Group.h"
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

bool GroupModel::removeAll(int userid) {
    std::string sql ="delete from groupuser where userid="+ std::to_string(userid) + ";";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }

    return mysql.update(sql);
}

std::vector<User> GroupModel::queryManagers(int groupid) {
    std::string sql = "select userid from groupuser where groupid =" + std::to_string(groupid) + " and (grouprole = 'owner' or grouprole = 'admin');";
    Mysql mysql;
    std::vector<User> users;
    if (!mysql.connect()) {
        return users;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) {
        return users;
    }
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)) != nullptr) {
        User user;
        user.setId(atoi(row[0]));
        users.push_back(user);
    }
    mysql_free_result(res);
    return users;
}

bool GroupModel::isInGroup(int userid, int groupid) {
    std::string sql ="select * from groupuser where userid="+ std::to_string(userid) + " and groupid=" + std::to_string(groupid) + ";";
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

bool GroupModel::isGroupExist(int groupid) {
    std::string sql ="select * from groupuser where groupid="+ std::to_string(groupid) + ";";
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

bool GroupModel::isManager(int userid, int groupid) {
    std::string sql = "select * from groupuser where userid=" + std::to_string(userid) + " and groupid=" + std::to_string(groupid) + " and (grouprole='owner' or grouprole='manager');";
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

Group GroupModel::query(int groupid) {
    Group group;
    std::string sql = "select id, groupname, groupdesc from allgroup where id=" + std::to_string(groupid) + ";";

    Mysql mysql;
    if (!mysql.connect()) {
        return group;
    }

    if (MYSQL_RES* res = mysql.query(sql)) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr) {
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
        }
        mysql_free_result(res);
    }
    return group;
}

std::string GroupModel::queryRole(int userid, int groupid) {
    std::string sql = "select grouprole from groupuser where userid=" + std::to_string(userid) + " and groupid=" + std::to_string(groupid) + ";";
    Mysql mysql;
    if (!mysql.connect()) return "";

    MYSQL_RES* res = mysql.query(sql);
    if(res == nullptr) return "";

    MYSQL_ROW row = mysql_fetch_row(res);
    std::string role;
    if (row != nullptr) role = row[0];

    mysql_free_result(res);
    return role;
}

bool GroupModel::leaveGroup(int userid, int groupid) {
    std::string sql = "delete from groupuser where userid=" + std::to_string(userid) + " and groupid=" + std::to_string(groupid) + ";";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

bool GroupModel::removeUser(int userid, int groupid) {
    std::string sql = "delete from groupuser where userid=" + std::to_string(userid) + " and groupid=" + std::to_string(groupid) + ";";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

bool GroupModel::updateRole(int userid, int groupid, const std::string& role) {
    std::string sql = "update groupuser set grouprole='" + role + "' where userid=" + std::to_string(userid) + " and groupid=" + std::to_string(groupid) + ";";
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }
    return mysql.update(sql);
}

bool GroupModel::removeGroup(int groupid) {
    Mysql mysql;
    if (!mysql.connect()) {
        return false;
    }

    mysql.update("delete from groupuser where groupid=" + std::to_string(groupid) + ";");
    mysql.update("delete from group_request where groupid=" + std::to_string(groupid) + ";");
    mysql.update("delete from group_message where groupid=" + std::to_string(groupid) + ";");
    return mysql.update("delete from allgroup where id=" + std::to_string(groupid) + ";");
}

bool GroupModel::isOwner(int userid, int groupid) {
    std::string sql = "select * from groupuser where userid=" + std::to_string(userid) + " and groupid=" + std::to_string(groupid) + " and grouprole='owner';";
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

bool GroupModel::isAdmin(int userid, int groupid) {
    std::string sql = "select * from groupuser where userid=" + std::to_string(userid) + " and groupid=" + std::to_string(groupid) + " and grouprole='admin';";
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

std::vector<GroupUser> GroupModel::queryUsers(int groupid) {
    std::string sql = "select u.id,u.name,gu.grouprole from groupuser gu inner join user u on gu.userid=u.id where gu.groupid=" + std::to_string(groupid) + ";";
    Mysql mysql;
    std::vector<GroupUser> users;
    if (!mysql.connect()) {
        return users;
    }

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return users;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        GroupUser user;

        user.user.setId(atoi(row[0]));
        user.user.setName(row[1]);
        user.setRole(row[2]);
        users.push_back(user);
    }
    mysql_free_result(res);
    return users;
}