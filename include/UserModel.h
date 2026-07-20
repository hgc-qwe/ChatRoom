#include "User.h"
#pragma once

class UserModel {
public:
    bool insert(User& user);

    User query(int userid);

    bool updateState(User user);

    bool restState();
};