#pragma once
#include <string>

class GroupMessage {
private:
    int id = 0;
    int groupid = 0;
    int userid = 0;
    std::string msg;
    std::string time;
public:
    GroupMessage(int id, int groupid, int userid, std::string msg, std::string time): id(id), groupid(groupid), userid(userid), msg(msg), time(time) {

    }

    GroupMessage() = default;

    int getId() const {
        return id;
    }
    void setId(const int id) {
        this->id = id;
    }

    int getGroupid() const {
        return groupid;
    }
    void setGroupid(const int groupid) {
        this->groupid = groupid;
    }

    int getUserid() const {
        return userid;
    }
    void setUserid(const int userid) {
        this->userid = userid;
    }

    std::string getMsg() const {
        return msg;
    }
    void setMsg(const std::string& msg) {
        this->msg = msg;
    }

    std::string getTime() const {
        return time;
    }
    void setTime(const std::string& time) {
        this->time = time;
    }
};