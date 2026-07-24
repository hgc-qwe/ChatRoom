cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)

# ============================================
# 公共源文件（Server 和 Client 共享的部分）
# ============================================
set(COMMON_SRC
    # base
    src/Buffer.cpp
    src/Util.cpp

    # codec
    src/MessageCodec.cpp

    # protobuf
    proto/chat.pb.cc
)

# ============================================
# testServer
# ============================================
add_executable(
    testServer

    test/testServer.cpp

    # network
    src/TcpServer.cpp
    src/TcpConnection.cpp
    src/EventLoop.cpp
    src/EventLoopThread.cpp
    src/EventLoopThreadPool.cpp
    src/Epoll.cpp
    src/Channel.cpp

    # dispatcher/service
    src/Dispatcher.cpp
    src/ChatService.cpp

    # model
    src/UserModel.cpp
    src/FriendModel.cpp
    src/GroupModel.cpp
    src/OfflineMsg.cpp

    # entity
    src/User.cpp
    src/Group.cpp

    # database
    src/Mysql.cpp

    # 公共文件
    ${COMMON_SRC}
)

target_include_directories(
    testServer
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/proto
)

target_link_libraries(
    testServer
    pthread
    mysqlclient
    protobuf::libprotobuf
)

# ============================================
# testClient
# ============================================
add_executable(
    testClient

    test/testClient.cpp

    # 客户端需要的源文件（根据你的项目调整）
    src/TcpConnection.cpp
    src/EventLoop.cpp
    src/EventLoopThread.cpp
    src/EventLoopThreadPool.cpp
    src/Epoll.cpp
    src/Channel.cpp

    # 公共文件
    ${COMMON_SRC}
)

target_include_directories(
    testClient
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/proto
)

target_link_libraries(
    testClient
    pthread
    protobuf::libprotobuf
)