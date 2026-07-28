#pragma once
#include <string>
#include "chat.pb.h"
#include "ChatService.h"
#include "TcpConnection.h"

class Dispatcher {
public:
    std::string dispatch(std::shared_ptr<TcpConnection> conn, const int msgid, const std::string& data);
};