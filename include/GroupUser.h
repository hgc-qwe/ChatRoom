#include <string>
#include "User.h"
#pragma once

class GroupUser {
private:
    std::string role;

public:
    User user;

    void setRole(const std::string& role);

    std::string getRole() const;
};