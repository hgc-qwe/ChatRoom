cmake_minimum_required(VERSION 3.10)

project(ChatRoom)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 头文件路径
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)


# testCodec
add_executable(testCodec
    test/testCodec.cpp
    src/MessageCodec.cpp
)