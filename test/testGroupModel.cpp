#include <iostream>
#include <vector>

#include "GroupModel.h"

int main()
{
    GroupModel model;

    // 创建群
    Group group("CppStudy", "learn cpp");

    if(model.createGroup(group))
    {
        std::cout << "create group success" << std::endl;
    }
    else
    {
        std::cout << "create group failed" << std::endl;
    }

    // 用户1加入群1
    model.addGroup(1, 1, "creator");

    // 用户2加入群1
    model.addGroup(2, 1, "normal");

    // 查询用户1所在群
    std::vector<Group> groups =
        model.queryGroups(1);

    std::cout << "\nuser1 groups:\n";

    for(auto &g : groups)
    {
        std::cout
            << g.getId() << " "
            << g.getName() << " "
            << g.getDesc()
            << std::endl;
    }

    // 查询群成员
    std::vector<int> users =
        model.queryGroupUsers(1, 1);

    std::cout << "\ngroup users:\n";

    for(auto id : users)
    {
        std::cout << id << std::endl;
    }

    return 0;
}