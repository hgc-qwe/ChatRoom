#include <cstring>
#include "MessageCodec.h"

std::string MessageCodec::encode(int msgid, const std::string& data) {
    std::string packet;
    int len = data.size();
    packet.append((char*)&msgid, sizeof(int));
    packet.append((char*)&len, sizeof(int));
    packet.append(data);
    return packet;
}

bool MessageCodec::decode(std::string& buffer, int& msgid, std::string& data) {
    if (buffer.size() < 8) return false;

    memcpy(&msgid, buffer.data(), sizeof(int));
    int len;
    memcpy(&len, buffer.data() + 4, sizeof(int));

    if (buffer.size() < 8 + len) return false;
    data = buffer.substr(8, len);
    buffer.erase(0, 8 + len);

    return true;
}