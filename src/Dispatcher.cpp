#include "Dispatcher.h"
#include "ChatService.h"
#include "MessageCodec.h"
#include "Logger.h"
#include <iostream>

std::string Dispatcher::dispatch(std::shared_ptr<TcpConnection> conn, int msgid, const std::string& data) {
    switch (msgid) {
        case chat::LOGIN_MSG: {
            chat::LoginReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("LoginReq parse failed");
                return "";
            }

            chat::LoginRes res;
            ChatService::instance()->login(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::LOGIN_MSG_ACK, response);
        }
        case chat::REG_MSG: {
            chat::RegisterReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("RegisterReq parse failed");
                return "";
            }

            chat::RegisterRes res;
            ChatService::instance()->reg(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::REG_MSG_ACK, response);
        }
        case chat::ADD_FRIEND_MSG: {
            chat::AddFriendReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("AddFriendReq parse failed");
                return "";
            }

            chat::AddFriendRes res;
            ChatService::instance()->addFriend(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::ADD_FRIEND_MSG_ACK, response);
        }
        case chat::ONE_CHAT_MSG: {
            chat::OneChatReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("OneChatReq parse failed");
                return "";
            }

            chat::OneChatRes res;
            ChatService::instance()->oneChat(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::ONE_CHAT_MSG_ACK, response);
        }
        case chat::CREATE_GROUP_MSG: {
            chat::CreateGroupReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("CreateGroupReq parse failed");
                return "";
            }

            chat::CreateGroupRes res;
            ChatService::instance()->createGroup(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::CREATE_GROUP_MSG_ACK, response);
        }
        case chat::GROUP_CHAT_MSG: {
            chat::GroupChatReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("GroupChatReq parse failed");
                return "";
            }

            chat::GroupChatRes res;
            ChatService::instance()->groupChat(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::GROUP_CHAT_MSG_ACK, response);
        }
        case chat::LOGOUT_MSG: {
            chat::LogoutReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("LogoutReq parse failed");
                return "";
            }

            chat::LogoutRes res;
            ChatService::instance()->logout(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::LOGOUT_MSG_ACK, response);
        }
        case chat::QUERY_FRIEND_REQ_MSG: {
            chat::QueryFriendReqReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("QueryFriendReqReq parse failed");
                return "";
            }
            chat::QueryFriendReqRes res;
            ChatService::instance()->queryFriendreq(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::QUERY_FRIEND_REQ_MSG_ACK, response);
        }
        case chat::ACCEPT_FRIEND_MSG: {
            chat::AcceptFriendReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("AcceptFriendReq parse failed");
                return "";
            }
            chat::AcceptFriendRes res;
            ChatService::instance()->acceptFriend(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::ACCEPT_FRIEND_MSG_ACK, response);
        }
        case chat::QUERY_FRIEND_MSG: {
            chat::QueryFriendReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("QueryFriendReq parse failed");
                return "";
            }
            chat::QueryFriendRes res;
            ChatService::instance()->queryFriend(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::QUERY_FRIEND_MSG_ACK, response);
        }
        case chat::DELETE_FRIEND_MSG: {
            chat::DeleteFriendReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("DeleteFriendReq parse failed");
                return "";
            }
            chat::DeleteFriendRes res;
            ChatService::instance()->deleteFriend(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::DELETE_FRIEND_MSG_ACK, response);
        }
        case chat::HISTORY_MSG: {
            chat::HistoryMsgReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("HistoryMsgReq parse failed");
                return "";
            }
            chat::HistoryMsgRes res;
            ChatService::instance()->queryHistoryMsg(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::HISTORY_MSG_ACK, response);
        }
        case chat::GROUP_HISTORY_MSG: {
            chat::GroupHistoryMsgReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("GroupHistoryMsgReq parse failed");
                return "";
            }
            chat::GroupHistoryMsgRes res;
            ChatService::instance()->queryGroupHistoryMsg(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::GROUP_HISTORY_MSG_ACK, response);
        }
        case chat::FILE_START_MSG: {
            chat::FileStartReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("FileStartReq parse failed");
                return "";
            }
            chat::FileStartRes res;
            ChatService::instance()->fileStart(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::FILE_START_MSG_ACK, response);
        }
        case chat::FILE_CHUNK_MSG: {
            chat::FileChunkReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("FileChunkReq parse failed");
                return "";
            }
            chat::FileChunkRes res;
            ChatService::instance()->fileChunk(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::FILE_CHUNK_MSG_ACK, response);
        }
        case chat::FILE_END_MSG: {
            chat::FileEndReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("FileEndReq parse failed");
                return "";
            }
            chat::FileEndRes res;
            ChatService::instance()->fileEnd(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::FILE_END_MSG_ACK, response);
        }
        case chat::DOWNLOAD_FILE_MSG: {
            chat::DownloadFileReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("DownloadFileReq parse failed");
                return "";
            }
            chat::DownloadFileRes res;
            ChatService::instance()->downloadFile(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::DOWNLOAD_FILE_MSG_ACK, response);
        }
        case chat::CANCEL_ACCOUNT_MSG: {
            chat::CancelAccountReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("CancelAccountReq parse failed");
                return "";
            }
            chat::CancelAccountRes res;
            ChatService::instance()->cancelAccount(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::CANCEL_ACCOUNT_MSG_ACK, response);
        }
        case chat::QUERY_GROUP_REQ_MSG: {
            chat::QueryGroupReqReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("QueryGroupReqReq parse failed");
                return "";
            }
            chat::QueryGroupReqRes res;
            ChatService::instance()->queryGroupreq(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::QUERY_GROUP_REQ_MSG_ACK, response);
        }
        case chat::ACCEPT_GROUP_MSG: {
            chat::AcceptGroupReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("AcceptGroupReq parse failed");
                return "";
            }
            chat::AcceptGroupRes res;
            ChatService::instance()->acceptGroup(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::ACCEPT_GROUP_MSG_ACK, response);
        }
        case chat::QUERY_GROUP_MSG: {
            chat::QueryGroupReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("QueryGroupReq parse failed");
                return "";
            }
            chat::QueryGroupRes res;
            ChatService::instance()->queryGroup(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::QUERY_GROUP_MSG_ACK, response);
        }
        case chat::APPLY_GROUP_MSG: {
            chat::ApplyGroupReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("ApplyGroupReq parse failed");
                return "";
            }
            chat::ApplyGroupRes res;
            ChatService::instance()->applyGroup(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::APPLY_GROUP_MSG_ACK, response);
        }
        case chat::LEAVE_GROUP_MSG: {
            chat::LeaveGroupReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("LeaveGroupReq parse failed");
                return "";
            }
            chat::LeaveGroupRes res;
            ChatService::instance()->leaveGroup(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::LEAVE_GROUP_MSG_ACK, response);
        }
        case chat::TRANSFER_OWNER_MSG: {
            chat::TransferOwnerReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("TransferOwnerReq parse failed");
                return "";
            }
            chat::TransferOwnerRes res;
            ChatService::instance()->transferOwner(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::TRANSFER_OWNER_MSG_ACK, response);
        }
        case chat::DISSOLVE_GROUP_MSG: {
            chat::DissolveGroupReq req;
            if (!req.ParseFromString(data)) {
                LOG_ERROR("DissolveGroupReq parse failed");
                return "";
            }
            chat::DissolveGroupRes res;
            ChatService::instance()->dissolveGroup(conn, req, res);
            std::string response;
            res.SerializeToString(&response);
            return MessageCodec::encode(chat::DISSOLVE_GROUP_MSG_ACK, response);
        }
        default: {
            LOG_ERROR("unknown message");
            return "";
        }
    }
}