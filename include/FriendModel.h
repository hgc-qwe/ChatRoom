#pragma once
#include <iostream>
#include <vector>
#include "User.h"


class FriendModel {
private:
    int userid;
    int friendid;

public:
    bool insert(int userid, int friendid);

    std::vector<User> query(int userid);

    bool isFriend(int fromid, int toid);

    bool remove(int userid, int friendid);

    bool removeAll(int userid);
};