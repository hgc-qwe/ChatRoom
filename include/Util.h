#pragma once
#include <string>

int setNonBlock(int fd);

std::string getCurrentTime();

std::string inputPassword();

std::string hashPassword(const std::string& password);

bool verifyPassword(const std::string& password, const std::string& stored);

bool isPasswordHashed(const std::string& stored);