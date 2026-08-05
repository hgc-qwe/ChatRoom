#pragma once
#include <string>

class GroupRequest {
public:
    GroupRequest(int groupid, int userid): groupid(groupid), userid(userid) {}

    int getUserid() {
        return userid;
    }

    int getGroupid() {
        return groupid;
    }
private:
    int userid;
    int groupid;
};