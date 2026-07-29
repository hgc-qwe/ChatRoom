#pragma once
#include <string>

class Message {
private:
    int id = 0;
    int fromid = 0;
    int toid = 0;
    std::string msg;
    std::string time;
public:
    Message(int id, int fromid, int toid, std::string msg, std::string time);
    Message() = default;

    int getId() const;
    void setId(const int id);

    int getFromid() const;
    void setFromid(const int fromid);

    int getToid() const;
    void setToid(const int toid);

    std::string getMsg() const;
    void setMsg(const std::string& msg);

    std::string getTime() const;
    void setTime(const std::string& time);
};