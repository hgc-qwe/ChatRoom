cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)


include_directories(
    include
    proto
)


# ======================
# Server
# ======================

add_executable(
    testServer

    test/testServer.cpp

    src/TcpServer.cpp
    src/Epoll.cpp

    src/MessageCodec.cpp
    src/Dispatcher.cpp
    src/ChatService.cpp

    src/User.cpp
    src/UserModel.cpp
    src/FriendModel.cpp
    src/Group.cpp
    src/GroupModel.cpp
    src/OfflineMsg.cpp
    src/Mysql.cpp

    proto/chat.pb.cc
)


target_link_libraries(
    testServer

    protobuf
    mysqlclient
)


# ======================
# Client
# ======================

add_executable(
    testClient

    test/testClient.cpp

    src/MessageCodec.cpp

    proto/chat.pb.cc
)


target_link_libraries(
    testClient

    protobuf
)