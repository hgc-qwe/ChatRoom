#pragma once
#include <vector>
#include <string>
#include "File.h"

class FileModel
{
public:

    bool insert(File file);

    std::vector<File> queryOffline(int userid);

    bool updateStatus(std::string fileid);

    File queryByFileid(std::string fileid);

    std::vector<File> queryFriendFile(const int userid, const int friendid);

    std::vector<File> queryGroupFile(const int groupid);
};