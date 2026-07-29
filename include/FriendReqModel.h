#pragma once
#include <vector>
#include "FriendRequest.h"

class FriendReqModel {
public:
    bool insert(int fromid, int toid);

    std::vector<FriendRequest> query(int userid);

    bool updateStatus(int fromid, int toid, int status);
};