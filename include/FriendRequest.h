#pragma once
#include <string>

class FriendRequest {
public:
    FriendRequest(int userid, int friendid): userid(userid), friendid(friendid) {}

    int getUserid() {
        return userid;
    }

    int getFriendid() {
        return friendid;
    }
private:
    int userid;
    int friendid;
};