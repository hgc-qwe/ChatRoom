#include <string>
#include <vector>
#include "File.h"
#include "FileModel.h"
#include "Mysql.h"

bool FileModel::insert(File file) {
    std::string sql = "insert into file(fileid, fromid, toid, filename, filesize, status, fromname, type, groupid) values('" + file.getFileid() + "'," + std::to_string(file.getFromid()) + "," + 
        std::to_string(file.getToid()) + ",'" + file.getFilename() + "'," + std::to_string(file.getFilesize()) + "," + std::to_string(file.getStatus()) + ",'" + file.getFromname() + "'," + std::to_string(file.getType()) + "," + std::to_string(file.getGroupid()) + ");";
    Mysql mysql;
    if (!mysql.connect()) return false;
    return mysql.update(sql);
}

std::vector<File> FileModel::queryOffline(int userid) {
    std::string sql = "select * from file where toid=" + std::to_string(userid) + " and status = 0 and type = 0;";
    std::vector<File> files;
    Mysql mysql;
    if (!mysql.connect()) return files;

    MYSQL_RES* res = mysql.query(sql);
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            File file;
            file.setFileid(row[0]);
            file.setFromid(std::stoi(row[1]));
            file.setToid(std::stoi(row[2]));
            file.setFilename(row[3]);
            file.setFilesize(std::stoull(row[4]));
            file.setStatus(std::stoi(row[5]));
            file.setFromname(row[6]);
            file.setType(std::stoi(row[7]));
            file.setGroupid(std::stoi(row[8]));
            files.push_back(file);
        }
        mysql_free_result(res);
    }
    return files;
}

bool FileModel::updateStatus(std::string fileid) {
    std::string sql = "update file set status = 1 where fileid ='" + fileid + "';";
    Mysql mysql;
    if (!mysql.connect()) return false;
    return mysql.update(sql);
}

File FileModel::queryByFileid(std::string fileid) {
    std::string sql = "select * from file where fileid='" + fileid + "';";
    File file;
    Mysql mysql;
    if (!mysql.connect()) return file;

    MYSQL_RES* res = mysql.query(sql);
    if (res) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            file.setFileid(row[0]);
            file.setFromid(std::stoi(row[1]));
            file.setToid(std::stoi(row[2]));
            file.setFilename(row[3]);
            file.setFilesize(std::stoull(row[4]));
            file.setStatus(std::stoi(row[5]));
            file.setFromname(row[6]);
            file.setType(std::stoi(row[7]));
            file.setGroupid(std::stoi(row[8]));
        }
        mysql_free_result(res);
    }
    return file;
}