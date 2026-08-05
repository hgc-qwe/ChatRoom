#include <vector>
#include "Group.h"
#include "User.h"
#pragma once

class GroupModel {
public:

    bool createGroup(Group &group);

    bool addGroup(int userid, int groupid, std::string role);

    std::vector<Group> queryGroups(int userid);

    std::vector<int> queryGroupUsers(int userid, int groupid);

    std::string queryRole(int userid, int groupid);

    bool removeAll(int userid);

    std::vector<User> queryManagers(int groupid);

    bool isInGroup(int userid, int groupid);

    bool isGroupExist(int groupid);

    bool isManager(int userid, int groupid);

    Group query(int groupid);
};