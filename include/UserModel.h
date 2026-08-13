#pragma once
#include "User.h"


class UserModel {
public:
    bool insert(User& user);

    User query(int userid);

    bool updateState(User user);

    bool restState();

    bool remove(int userid);

    User queryByEmail(const std::string& email);
};