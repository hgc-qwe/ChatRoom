#include <iostream>
#include <vector>
#include "User.h"
#pragma once

class FriendModel {
private:
    int userid;
    int friendid;

public:
    bool insert(int userid, int friendid);

    std::vector<User> query(int userid);
};