#include <string>
#include <hiredis/hiredis.h>
#include "Redis.h"

Redis::Redis() {
    context = nullptr;
}

Redis::~Redis() {
    if (context) {
        redisFree((redisContext*)context);
    }
}

bool Redis::connect() {
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr) return false;
    if (c->err) {
        redisFree(c);
        return false;
    }

    context = c;
    return true;
}

bool Redis::set(const std::string& key, const std::string& value) {
    redisReply* reply = (redisReply*)redisCommand((redisContext*)context, "SET %s %s", key.c_str(), value.c_str());

    if(reply == nullptr) return false;

    freeReplyObject(reply);
    return true;
}

bool Redis::get(const std::string& key, std::string value) {
    redisReply* reply = (redisReply*)redisCommand((redisContext*)context, "GET %s", key.c_str());

    if(reply == nullptr) return false;
    if(reply->type == REDIS_REPLY_STRING) value = reply->str;

    freeReplyObject(reply);
    return true;
}

bool Redis::del(const std::string& key) {
    redisReply* reply = (redisReply*)redisCommand((redisContext*)context, "DEL %s", key.c_str());

    if(reply == nullptr) return false;
    bool success = false;
    if(reply->type == REDIS_REPLY_STRING) success = (reply->integer > 0);

    freeReplyObject(reply);
    return success;
}

bool Redis::pushList(const std::string& key, const std::string& value) {
    redisReply* reply = (redisReply*)redisCommand((redisContext*)context, "RPUSH %s %s", key.c_str(), value.c_str());

    if(reply == nullptr) return false;
    bool ret = reply->type != REDIS_REPLY_ERROR;

    freeReplyObject(reply);
    return true;
}

bool Redis::getList(const std::string& key, std::vector<std::string>& values) {
    redisReply* reply = (redisReply*)redisCommand((redisContext*)context, "LRANGE %s 0 -1", key.c_str());
    if(reply == nullptr) return false;
    
    for (size_t i = 0; i < reply->elements; i++) {
        values.push_back(reply->element[i]->str);
    }

    freeReplyObject(reply);
    return true;
}