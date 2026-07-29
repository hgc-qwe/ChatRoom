#pragma once
#include <vector>
#include <string>
#include "Message.h"

class MessageModel {
public:
    bool insert(int fromid, int toid, std::string msg);

    std::vector<Message> queryOffline(int userid);

    bool updateStatus(int userid);

    std::vector<Message> queryHistory(int userid, int friendid);
};