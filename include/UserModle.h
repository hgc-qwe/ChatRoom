#include "User.h"

class UserModle {
public:
    bool insert(User user);

    User query(int userid);

    bool updateState(User user);

    bool restState();
};