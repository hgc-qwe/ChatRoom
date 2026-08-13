#pragma once
#include <string>


class User {
private:
    int Id = -1;
    std::string Name;
    std::string Password;
    std::string State;
    std::string Email;

public:
    void setId(const int id);
    int getId() const;

    void setName(const std::string& name);
    std::string getName() const;

    void setPassword(const std::string& password);
    std::string getPassword() const;

    void setState(const std::string& state);
    std::string getState() const;

    void setEmail(const std::string& email);
    std::string getEmail() const;
};