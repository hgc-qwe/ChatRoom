#include <cstring>
#include <iostream>
#include <cstdint>
#include "MessageCodec.h"

std::string MessageCodec::encode(int msgid, const std::string& data) {
    std::string packet;
    u_int32_t len = data.size();
    u_int32_t id = msgid;
    packet.append((char*)&id, sizeof(id));
    packet.append((char*)&len, sizeof(len));
    packet.append(data);
    return packet;
}

bool MessageCodec::decode(Buffer& buffer, int& msgid, std::string& data) {
    if (buffer.readableBytes() < 8) return false;

    const char* readPtr = buffer.beginRead();
    uint32_t id;
    uint32_t len;

    memcpy(&id, readPtr, sizeof(uint32_t));
    memcpy(&len, readPtr + sizeof(uint32_t), sizeof(uint32_t));

    if(len > 2 * 1024 * 1024)
    {
        std::cout << "invalid packet!" << std::endl;
        return false;
    }

    if (buffer.readableBytes() < 8 + len) return false;
    msgid = static_cast<int>(id);
    buffer.retrieve(8);
    data.assign(buffer.beginRead(), len);
    buffer.retrieve(len);

    return true;
}