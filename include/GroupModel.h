#include <vector>
#include "Group.h"
#pragma once

class GroupModel {
public:

    bool createGroup(Group &group);

    bool addGroup(int userid, int groupid, std::string role);

    std::vector<Group> queryGroups(int userid);

    std::vector<int> queryGroupUsers(int userid, int groupid);
};