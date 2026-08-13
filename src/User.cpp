#include <iostream>
#include <string>
#include "User.h"

void User::setId(const int id) {
    Id = id;
}
int User::getId() const {
    return Id;
}

void User::setName(const std::string& name) {
    Name = name;
}
std::string User::getName() const {
    return Name;
}

void User::setPassword(const std::string& password) {
    Password = password;
}
std::string User::getPassword() const {
    return Password;
}

void User::setState(const std::string& state) {
    State = state;
}
std::string User::getState() const {
    return State;
}

void User::setEmail(const std::string& email) {
    Email = email;
}
    
std::string User::getEmail() const {
    return Email;
}