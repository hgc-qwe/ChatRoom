#include <iostream>

#include "Dispatcher.h"
#include "chat.pb.h"


int main()
{
    chat::LoginReq req;

    req.set_id(1);
    req.set_password("123");


    std::string data;

    // protobuf序列化
    req.SerializeToString(&data);


    Dispatcher dispatcher;


    // 模拟收到登录消息
    dispatcher.dispatch(
        chat::LOGIN_MSG,
        data
    );


    return 0;
}