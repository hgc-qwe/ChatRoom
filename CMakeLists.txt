cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

configure_file(
    ${PROJECT_SOURCE_DIR}/config.json
    ${PROJECT_BINARY_DIR}/config.json
    COPYONLY
)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)
set(HIREDIS_LIB /usr/lib/x86_64-linux-gnu/libhiredis.so)

# ============================================
# 公共源文件（Server 和 Client 共享的部分）
# ============================================
set(COMMON_SRC

    src/Buffer.cpp
    src/Util.cpp
    src/Logger.cpp
    src/MessageCodec.cpp
    proto/chat.pb.cc
)

# ============================================
# testServer
# ============================================
add_executable(
    testServer

    test/testServer.cpp


    src/TcpServer.cpp
    src/TcpConnection.cpp
    src/EventLoop.cpp
    src/EventLoopThread.cpp
    src/EventLoopThreadPool.cpp
    src/Epoll.cpp
    src/Channel.cpp
    src/Dispatcher.cpp
    src/ChatService.cpp
    src/UserModel.cpp
    src/FriendModel.cpp
    src/GroupModel.cpp
    src/GroupMessageModel.cpp
    src/GroupUser.cpp
    src/OfflineMsg.cpp
    src/FriendReqModel.cpp
    src/Message.cpp
    src/MessageModel.cpp
    src/FileModel.cpp
    src/GroupReqModel.cpp
    src/BlacklistModel.cpp
    src/User.cpp
    src/Group.cpp
    src/Mysql.cpp
    src/Redis.cpp
    src/EmailSender.cpp

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
    spdlog::spdlog
    ${HIREDIS_LIB}
    CURL::libcurl
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# ============================================
# testClient
# ============================================
add_executable(
    testClient

    test/testClient.cpp

    src/TcpConnection.cpp
    src/EventLoop.cpp
    src/EventLoopThread.cpp
    src/EventLoopThreadPool.cpp
    src/Epoll.cpp
    src/Channel.cpp


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
    spdlog::spdlog
    CURL::libcurl
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)