#include<iostream>
#include "MessageCodec.h"


int main()
{

    std::string data="hello protobuf";


    auto packet =
    MessageCodec::encode(
        1,
        data
    );


    std::string buffer=packet;


    int msgid;

    std::string result;


    if(MessageCodec::decode(
        buffer,
        msgid,
        result))
    {

        std::cout
        <<"msgid:"
        <<msgid
        <<std::endl;


        std::cout
        <<"data:"
        <<result
        <<std::endl;

    }

}