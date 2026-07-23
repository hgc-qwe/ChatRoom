#pragma once
#include <string>

class Buffer {
public:
    Buffer(size_t size = 1024);

    size_t readableBytes() const;

    size_t writableBytes() const;

    char* begin();

    char* beginWrite();

    char* beginRead();

    void hasWritten(size_t len);

    void retrieve(size_t len);

    std::string retrieveAllAsString();

    void append(const char* data, size_t len);

    void append(const std::string& str);

    bool empty() const;

    const char* data() const;

    size_t size() const;

    void erase(size_t pos, size_t len);

    std::string& getString();

    std::string retrieveAll();
private:
    std::string buffer;
    size_t readIndex;
    size_t writeIndex;
};