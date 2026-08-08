#pragma once
#include <string>

class FriendRequest {
private:
    int fromid;
    int toid;

public:
    FriendRequest(int fromid, int toid) : fromid(fromid), toid(toid) {}

    int getFromid() const {
        return fromid;
    }

    int getToid() const {
        return toid;
    }
};