#include "User.h"
#include "UserModel.h"

#include <iostream>

int main()
{
    UserModel model;

    User user;

    //user.setName("zhangsan");
    //user.setPassword("123456");
    //user.setState("offline");

    //model.insert(user);

    User result = model.query(0);

    std::cout
        << result.getName()
        << std::endl;

    return 0;
}