#pragma once
#include <string>

class MessageCodec {
public:
    static std::string encode(int msgid, const std::string& data);

    static bool decode(std::string& buffer, int& msgid, std::string& data);
};