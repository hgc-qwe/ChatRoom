#pragma once
#include <vector>
#include "Group.h"
#include "User.h"

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

    bool leaveGroup(int userid, int groupid);

    bool removeUser(int userid, int groupid);

    bool updateRole(int userid, int groupid, const std::string& role);

    bool removeGroup(int groupid);

    bool isOwner(int userid, int groupid);

    bool isAdmin(int userid, int groupid);

    std::vector<GroupUser> queryUsers(int groupid);

    std::vector<Group> queryGroupId(const int userid);
};