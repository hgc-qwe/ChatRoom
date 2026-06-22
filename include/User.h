#include <string>
#pragma once

class User {
private:
    int Id;
    std::string Name;
    std::string Password;
    std::string State;

public:
    void setId(int id);
    int getId();

    void setName(std::string name);
    std::string getName();

    void setPassword(std::string password);
    std::string getPassword();

    void setState(bool state);
    std::string getState();
};