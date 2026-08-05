#pragma once
#include <vector>
#include "GroupRequest.h"

class GroupReqModel {
public:
    bool insert(int groupid, int userid);

    std::vector<GroupRequest> query(int userid);

    bool update(int groupid, int userid, int status);

    bool remove(int groupid, int userid);

    bool removeAll(int userid);

    bool isApplied(int userid, int groupid);
};