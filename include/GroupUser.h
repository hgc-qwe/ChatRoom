#include <string>
#include "User.h"
#pragma once

class GroupUser {
private:
    std::string Role;

public:
    User user;

    void setRole(std::string& role);

    std::string getRole();
};