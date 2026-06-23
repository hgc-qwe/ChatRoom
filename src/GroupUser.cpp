#include <iostream>
#include "GroupUser.h"

void GroupUser::setRole(std::string& role) {
    Role = role;
}

std::string GroupUser::getRole() {
    return Role;
}