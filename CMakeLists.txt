cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# ======================
# include目录
# ======================

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/proto
)


# ======================
# protobuf
# ======================

find_package(Protobuf REQUIRED)

include_directories(
    ${Protobuf_INCLUDE_DIRS}
)


# ======================
# mysql
# ======================

find_package(PkgConfig REQUIRED)

pkg_check_modules(MYSQL REQUIRED mysqlclient)

include_directories(
    ${MYSQL_INCLUDE_DIRS}
)



# ======================
# 公共源码
# ======================

set(COMMON_SRC

    src/MessageCodec.cpp

    src/Buffer.cpp

    proto/chat.pb.cc

)



# ======================
# server
# ======================

set(SERVER_SRC

    test/testServer.cpp

    src/Channel.cpp
    src/EventLoop.cpp
    src/Epoll.cpp

    src/TcpServer.cpp
    src/TcpConnection.cpp

    src/Dispatcher.cpp
    src/ChatService.cpp

    src/User.cpp
    src/UserModel.cpp

    src/FriendModel.cpp

    src/Group.cpp
    src/GroupUser.cpp
    src/GroupModel.cpp

    src/OfflineMsg.cpp

    src/Mysql.cpp

    src/Util.cpp

    ${COMMON_SRC}
)


add_executable(testServer
    ${SERVER_SRC}
)


target_link_libraries(testServer

    protobuf

    ${MYSQL_LIBRARIES}

    pthread
)



# ======================
# client
# ======================


set(CLIENT_SRC

    test/testClient.cpp

    ${COMMON_SRC}

)



add_executable(testClient

    ${CLIENT_SRC}

)


target_link_libraries(testClient

    protobuf

    pthread

)