#pragma once
#include <vector>
#include <string>


class OfflineMsg {
public:
    bool insert(int toid, int userid, std::string msg);

    bool remove(int userid);

    std::vector<std::string> query(int userid);
};