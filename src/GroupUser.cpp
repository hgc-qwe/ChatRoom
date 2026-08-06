#include <iostream>
#include <string>
#include "GroupUser.h"

void GroupUser::setRole(const std::string& role) {
    this->role = role;
}

std::string GroupUser::getRole() const {
    return role;
}