#include <cstring>
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
    memcpy(&msgid, readPtr, sizeof(int));
    int len;
    memcpy(&len, readPtr + 4, sizeof(int));

    if (buffer.readableBytes() < 8 + len) return false;
    buffer.retrieve(8);
    data.assign(buffer.beginRead(), len);
    buffer.retrieve(len);

    return true;
}