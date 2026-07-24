#include <cstring>
#include "Buffer.h"

Buffer::Buffer(size_t size) {
    buffer.resize(size);
    readIndex = 0;
    writeIndex = 0;
}

size_t Buffer::readableBytes() const {
    return writeIndex - readIndex;
}

size_t Buffer::writableBytes() const {
    return buffer.size() - writeIndex;
}

char* Buffer::begin() {
    return buffer.data();
}

char* Buffer::beginWrite() {
    return buffer.data() + writeIndex;
}

const char* Buffer::beginRead() const {
    return buffer.data() + readIndex;
}

void Buffer::hasWritten(size_t len) {
    writeIndex += len;
}

void Buffer::retrieve(size_t len) {
    if (len < readableBytes()) {
        readIndex += len;
    } else {
        readIndex = 0;
        writeIndex = 0;
    }
}

std::string Buffer::retrieveAllAsString() {
    std::string str(beginRead(), readableBytes());
    retrieve(readableBytes());
    return str;
}

void Buffer::append(const char* data, size_t len) {
    if (writableBytes() < len) buffer.resize(writeIndex + len);
    memcpy(beginWrite(), data, len);
    writeIndex += len;
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.size());
}

bool Buffer::empty() const {
    return buffer.empty();
}

const char* Buffer::data() const {
    return buffer.data();
}

size_t Buffer::size() const {
    return buffer.size();
}

void Buffer::erase(size_t pos, size_t len) {
    buffer.erase(pos, len);
}

std::string& Buffer::getString() {
    return buffer;
}

std::string Buffer::retrieveAll() {
    std::string str(beginRead(), readableBytes());
    retrieve(readableBytes());
    return str;
}