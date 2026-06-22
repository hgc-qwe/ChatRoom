cmake_minimum_required(VERSION 3.10)

project(usermodel)

include_directories(include)

add_executable(testUserModel test/testUserModel.cpp src/User.cpp src/UserModel.cpp src/Mysql.cpp)

target_link_libraries(testUserModel mysqlclient)