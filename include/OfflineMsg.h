#include <vector>
#include <string>
#pragma once

class OfflineMsg {
public:
    bool insert(int userid, std::string msg);

    bool remove(int userid);

    std::vector<std::string> query(int userid);
};