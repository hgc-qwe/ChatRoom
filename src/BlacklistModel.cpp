#include <string>
#include "BlacklistModel.h"
#include "Mysql.h"

bool BlacklistModel::insert(int userid, int blackid) {
    std::string sql = "insert into blacklist(userid, blackid) values(" + std::to_string(userid) + "," + std::to_string(blackid) + ");";
    Mysql mysql;
    if (!mysql.connect()) return false;
    return mysql.update(sql);
}

bool BlacklistModel::remove(int userid, int blackid) {
    std::string sql = "delete from blacklist where userid=" + std::to_string(userid) + " and blackid=" + std::to_string(blackid) + ";";
    Mysql mysql;
    if (!mysql.connect()) return false;
    return mysql.update(sql);
}

bool BlacklistModel::isBlacked(int userid, int blackid) {
    std::string sql = "select * from blacklist where userid=" + std::to_string(userid) + " and blackid=" + std::to_string(blackid) + ";";
    Mysql mysql;
    if (!mysql.connect()) return false;

    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return false;

    bool exist = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exist;
}