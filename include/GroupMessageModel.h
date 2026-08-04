#pragma once
#include <vector>
#include "GroupMessage.h"

class GroupMessageModel {
public:
    bool insert(int groupid, int userid, std::string msg, std::string username);

    std::vector<GroupMessage> query(int groupid);
};