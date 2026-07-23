cmake_minimum_required(VERSION 3.10)

project(ChatRoom)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# =========================
# include目录
# =========================

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/proto
)


# =========================
# protobuf
# =========================

find_package(Protobuf REQUIRED)

include_directories(${Protobuf_INCLUDE_DIRS})


# =========================
# mysql
# =========================

find_package(PkgConfig REQUIRED)

pkg_check_modules(MYSQL REQUIRED mysqlclient)



# =========================
# server源码
# =========================

file(GLOB SERVER_SRC

    ${PROJECT_SOURCE_DIR}/src/*.cpp

    ${PROJECT_SOURCE_DIR}/proto/chat.pb.cc

)


add_executable(testServer
    test/testServer.cpp
    ${SERVER_SRC}
    proto/chat.pb.cc
)



target_link_libraries(testServer
    protobuf
    ${MYSQL_LIBRARIES}
    pthread
)



# =========================
# client
# =========================

add_executable(testClient
    test/testClient.cpp
    src/MessageCodec.cpp
    proto/chat.pb.cc
)



target_link_libraries(testClient
    protobuf
    pthread
)