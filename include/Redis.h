#pragma once
#include <string>
#include <vector>
#include <functional>

class Redis{
public:
    Redis();
    ~Redis();

    bool connect();

    bool set(const std::string& key, const std::string& value);

    bool get(const std::string& key, std::string value);

    bool del(const std::string& key);

    bool pushList(const std::string& key, const std::string& value);

    bool getList(const std::string& key, std::vector<std::string>& values);
private:
    void* context;
};