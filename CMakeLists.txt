cmake_minimum_required(VERSION 3.10)

project(ChatRoom)


set(CMAKE_CXX_STANDARD 17)


# 头文件路径
include_directories(
    include
    proto
)


# protobuf头文件
include_directories(
    /usr/include
)


# 测试Dispatcher

add_executable(testDispatcher

    test/testDispatcher.cpp

    src/Dispatcher.cpp
    src/ChatService.cpp

    src/User.cpp
    src/UserModel.cpp

    src/Group.cpp
    src/FriendModel.cpp
    src/GroupModel.cpp

    src/OfflineMsg.cpp

    src/Mysql.cpp

    proto/chat.pb.cc
)


# mysql
target_link_libraries(testDispatcher
    mysqlclient
    protobuf
)