#pragma once
#include <string>
#include "chat.pb.h"
#include "ChatService.h"

class Dispatcher {
public:
    std::string dispatch(const chat::MsgTyp msgid, const std::string& data);
};