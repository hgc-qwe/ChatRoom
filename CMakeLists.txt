cmake_minimum_required(VERSION 3.10)

project(ChatRoom)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    include
    proto
)

find_package(Protobuf REQUIRED)

add_executable(testProto
    test/testProto.cpp
    proto/chat.pb.cc
)

target_link_libraries(testProto
    protobuf
)