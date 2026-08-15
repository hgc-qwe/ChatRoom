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


set(COMMON_SRC

    src/Buffer.cpp
    src/Util.cpp
    src/Logger.cpp
    src/MessageCodec.cpp
    proto/chat.pb.cc
)


add_executable(
    Server

    src/Server.cpp

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
    Server
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/proto
)

target_link_libraries(
    Server
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


add_executable(
    Client

    src/Client.cpp

    src/TcpConnection.cpp
    src/EventLoop.cpp
    src/EventLoopThread.cpp
    src/EventLoopThreadPool.cpp
    src/Epoll.cpp
    src/Channel.cpp


    ${COMMON_SRC}
)

target_include_directories(
    Client
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/proto
)

target_link_libraries(
    Client
    pthread
    protobuf::libprotobuf
    spdlog::spdlog
    CURL::libcurl
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)