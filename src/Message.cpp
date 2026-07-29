#include <string>
#include "Message.h"

Message::Message(int id, int fromid, int toid, std::string msg, std::string time): id(id), fromid(fromid), toid(toid), msg(msg), time(time) {

}

int Message::getId() const {
    return id;
}

void Message::setId(const int id) {
    this->id = id;
}

int Message::getFromid() const {
    return fromid;
}

void Message::setFromid(const int fromid) {
    this->fromid = fromid;
}

int Message::getToid() const {
    return toid;
}

void Message::setToid(const int toid) {
    this->toid = toid;
}

std::string Message::getMsg() const {
    return msg;
}

void Message::setMsg(const std::string& msg) {
    this->msg = msg;
}

std::string Message::getTime() const {
    return time;
}

void Message::setTime(const std::string& time) {
    this->time = time;
}