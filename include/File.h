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
    std::string fromname;
public:
    File(std::string fileid, int fromid, int toid, std::string filename, uint64_t filesize, int status, std::string fromname): fileid(fileid), fromid(fromid), toid(toid), filename(filename), filesize(filesize), status(status), fromname(fromname) {}
    File() = default;

    std::string getFileid() const {
        return fileid;
    }
    void setFileid(const std::string& fileid) {
        this->fileid = fileid;
    }

    int getFromid() const {
        return fromid;
    }
    void setFromid(const int fromid) {
        this->fromid = fromid;
    }

    int getToid() const {
        return toid;
    }
    void setToid(const int toid) {
        this->toid = toid;
    }

    std::string getFilename() const {
        return filename;
    }
    void setFilename(const std::string& filename) {
        this->filename = filename;
    }

    uint64_t getFilesize() const {
        return filesize;
    }
    void setFilesize(const uint64_t filesize) {
        this->filesize = filesize;
    }

    int getStatus() const {
        return status;
    }
    void setStatus(const int status) {
        this->status = status;
    }

    std::string getFromname() const {
        return fromname;
    }
    void setFromname(const std::string& fromname) {
        this->fromname = fromname;
    }
};