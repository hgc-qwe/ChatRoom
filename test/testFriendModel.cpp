#include <iostream>
#include <vector>

#include "FriendModel.h"
#include "User.h"

int main()
{
    FriendModel model;

    if(model.insert(1, 2))
    {
        std::cout << "add friend success" << std::endl;
    }
    else
    {
        std::cout << "add friend failed" << std::endl;
    }

    std::cout << "\nfriend list:\n";

    std::vector<User> friends = model.query(1);

    for(auto &user : friends)
    {
        std::cout
            << user.getId() << " "
            << user.getName() << " "
            << user.getState()
            << std::endl;
    }

    return 0;
}