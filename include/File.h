#pragma once
#include <cstdint>
#include <string>

class File {
private:
    std::string fileid;
    int fromid;
    int toid;
    std::string filename;
    uint64_t filesize;
    int status;
public:
    File(std::string fileid, int fromid, int toid, std::string filename, uint64_t filesize, int status): fileid(fileid), fromid(fromid), toid(toid), filename(filename), filesize(filesize), status(status) {}
    File() = default;

    std::string getFileid() {
        return fileid;
    }
    void setFileid(const std::string& fileid) {
        this->fileid = fileid;
    }

    int getFromid() {
        return fromid;
    }
    void setFromid(const int fromid) {
        this->fromid = fromid;
    }

    int getToid() {
        return toid;
    }
    void setToid(const int toid) {
        this->toid = toid;
    }

    std::string getFilename() {
        return filename;
    }
    void setFilename(const std::string& filename) {
        this->filename = filename;
    }

    uint64_t getFilesize() {
        return filesize;
    }
    void setFilesize(const uint64_t filesize) {
        this->filesize = filesize;
    }

    int getStatus() {
        return status;
    }
    void setStatus(const int status) {
        this->status = status;
    }
};