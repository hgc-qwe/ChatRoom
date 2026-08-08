#pragma once
#include <string>

class BlacklistModel {
public:
    bool insert(int userid, int blackid);

    bool remove(int userid, int blackid);

    bool isBlacked(int userid, int blackid);
};