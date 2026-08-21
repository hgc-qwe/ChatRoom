#include <vector>
#include <string>
#include "FriendReqModel.h"
#include "FriendRequest.h"
#include "Mysql.h"
#include "MysqlPool.h"

bool FriendReqModel::insert(int fromid, int toid) {
    std::string sql = "insert into friend_request(fromid, toid) values(" + std::to_string(fromid) + "," + std::to_string(toid) + ");";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }
    return mysql.update(sql);
}

std::vector<FriendRequest> FriendReqModel::query(int userid) {
    std::string sql = "select fromid from friend_request where toid=" + std::to_string(userid) + " and status=0;";
    auto mysql = MysqlPool::instance()->acquire();
    std::vector<FriendRequest> requests;
    if (!mysql) return requests;
    MYSQL_RES* res = mysql.query(sql);
    if (res == nullptr) return requests;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        int fromid = atoi(row[0]);
        requests.emplace_back(fromid, userid);
    }
    mysql_free_result(res);
    return requests;
}

bool FriendReqModel::updateStatus(int fromid, int toid, int status) {
    std::string sql = "update friend_request set status=" + std::to_string(status) + " where fromid=" + std::to_string(fromid) + " and toid=" + std::to_string(toid) + ";";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }
    return mysql.update(sql);
}

bool FriendReqModel::removeAll(int userid) {
    std::string sql ="delete from friend_request where fromid="+ std::to_string(userid) + " or toid=" + std::to_string(userid) + ";";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
        return false;
    }

    return mysql.update(sql);
}

bool FriendReqModel::isApplied(int fromid, int toid) {
    std::string sql = "select * from friend_request where fromid=" + std::to_string(fromid) + " and toid=" + std::to_string(toid) + " and status=0;";
    auto mysql = MysqlPool::instance()->acquire();
    if (!mysql) {
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