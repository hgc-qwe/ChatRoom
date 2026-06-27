#include <iostream>
#include "../proto/chat.pb.h"

int main()
{
    chat::LoginReq req;

    req.set_msgid(chat::Login_msg);
    req.set_userid(1001);
    req.set_password("123456");

    std::string buffer;

    if(req.SerializeToString(&buffer))
    {
        std::cout << "serialize success\n";
    }

    chat::LoginReq req2;

    if(req2.ParseFromString(buffer))
    {
        std::cout << req2.msgid() << std::endl;
        std::cout << req2.userid() << std::endl;
        std::cout << req2.password() << std::endl;
    }

    return 0;
}