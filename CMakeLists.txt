cmake_minimum_required(VERSION 3.10)

project(ChatRoom)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ==========================
# include路径
# ==========================

include_directories(
    include
    proto
)


# ==========================
# protobuf
# ==========================

find_package(Protobuf REQUIRED)


# ==========================
# Server
# ==========================

add_executable(testServer

    test/testServer.cpp


    # Reactor
    src/Epoll.cpp
    src/EventLoop.cpp
    src/Channel.cpp


    # Network
    src/TcpServer.cpp
    src/TcpConnection.cpp


    # Codec
    src/MessageCodec.cpp


    # Dispatcher
    src/Dispatcher.cpp


    # Service
    src/ChatService.cpp


    # User
    src/User.cpp
    src/UserModel.cpp


    # Friend
    src/FriendModel.cpp


    # Group
    src/Group.cpp
    src/GroupModel.cpp


    # Offline
    src/OfflineMsg.cpp


    # Mysql
    src/Mysql.cpp


    # protobuf
    proto/chat.pb.cc
)



target_link_libraries(testServer

    protobuf::libprotobuf

    mysqlclient

    pthread
)



# ==========================
# Client
# ==========================

add_executable(testClient


    test/testClient.cpp


    src/MessageCodec.cpp


    proto/chat.pb.cc

)



target_link_libraries(testClient

    protobuf::libprotobuf

    pthread

)