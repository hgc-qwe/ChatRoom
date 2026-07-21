#include "Dispatcher.h"
#include "ChatService.h"
#include "MessageCodec.h"
#include <iostream>

std::string Dispatcher::dispatch(int msgid, const std::string& data) {
    switch (msgid) {
        case chat::LOGIN_MSG: {
            chat::LoginReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "LoginReq parse failed" << std::endl;
                return "";
            }

            chat::LoginRes res;
            ChatService::instance()->login(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::LOGIN_MSG_ACK, response);
        }
        case chat::REG_MSG: {
            chat::RegisterReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "RegisterReq parse failed" << std::endl;
                return "";
            }

            chat::RegisterRes res;
            ChatService::instance()->reg(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::REG_MSG_ACK, response);
        }
        case chat::ADD_FRIEND_MSG: {
            chat::AddFriendReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "AddFriendReq parse failed" << std::endl;
                return "";
            }

            chat::AddFriendRes res;
            ChatService::instance()->addFriend(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::ADD_FRIEND_MSG_ACK, response);
        }
        case chat::ONE_CHAT_MSG: {
            chat::OneChatReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "OneChatReq parse failed" << std::endl;
                return "";
            }

            chat::OneChatRes res;
            ChatService::instance()->oneChat(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::ONE_CHAT_MSG_ACK, response);
        }
        case chat::CREATE_GROUP_MSG: {
            chat::CreateGroupReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "CreateGroupReq parse failed" << std::endl;
                return "";
            }

            chat::CreateGroupRes res;
            ChatService::instance()->createGroup(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::CREATE_GROUP_MSG_ACK, response);
        }
        case chat::ADD_GROUP_MSG: {
            chat::AddGroupReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "AddGroupReq parse failed" << std::endl;
                return "";
            }

            chat::AddGroupRes res;
            ChatService::instance()->addGroup(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::ADD_GROUP_MSG_ACK, response);
        }
        case chat::GROUP_CHAT_MSG: {
            chat::GroupChatReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "GroupChatReq parse failed" << std::endl;
                return "";
            }

            chat::GroupChatRes res;
            ChatService::instance()->groupChat(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::GROUP_CHAT_MSG_ACK, response);
        }
        case chat::LOGOUT_MSG: {
            chat::LogoutReq req;
            if (!req.ParseFromString(data)) {
                std::cout << "LogoutReq parse failed" << std::endl;
                return "";
            }

            chat::LogoutRes res;
            ChatService::instance()->loginout(req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::LOGOUT_MSG_ACK, response);
        }
        default: {
            std::cout << "unknown message" << std::endl;
            return "";
        }
    }
}