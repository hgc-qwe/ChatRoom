#pragma once
#include <string>
#include "Buffer.h"

class MessageCodec {
public:
    static std::string encode(int msgid, const std::string& data);

    static bool decode(Buffer& buffer, int& msgid, std::string& data);
};