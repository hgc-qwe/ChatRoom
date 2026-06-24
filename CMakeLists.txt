cmake_minimum_required(VERSION 3.10)

project(test)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

find_library(MYSQLCLIENT mysqlclient)

# FriendModel测试
add_executable(
    testFriendModel
    test/testFriendModel.cpp
    src/User.cpp
    src/Mysql.cpp
    src/FriendModel.cpp
)

target_link_libraries(
    testFriendModel
    ${MYSQLCLIENT}
)

# GroupModel测试
add_executable(
    testGroupModel
    test/testGroupModel.cpp
    src/Group.cpp
    src/GroupUser.cpp
    src/Mysql.cpp
    src/GroupModel.cpp
)

target_link_libraries(
    testGroupModel
    ${MYSQLCLIENT}
)