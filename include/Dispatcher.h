#pragma once
#include "chat.pb.h"
#include "ChatService.h"

class Dispatcher {
public:
    void dispatch(const chat::MsgTyp msgid, const std::string& data);
};