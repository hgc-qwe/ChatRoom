#include <string>
#include <vector>
#include "GroupUser.h"
#pragma once

class Group {
private:
    int Id;
    std::string Name;
    std::string Desc;
    std::vector<GroupUser> users;

public:
    Group(const int id, const std::string& name, const std::string desc);

    void setId(int id);
    int getId();

    void setName(std::string name);
    std::string getName();

    void setDesc(std::string desc);
    std::string getDesc();

    std::vector<GroupUser> getUsers();
};