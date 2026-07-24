cmake_minimum_required(VERSION 3.10)

project(ChatRoom)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(
    testEventLoop

    test/testEventLoop.cpp

    src/EventLoop.cpp
    src/EventLoopThread.cpp
    src/Channel.cpp
    src/Epoll.cpp
)


target_include_directories(
    testEventLoop
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(
    testEventLoop
    pthread
)