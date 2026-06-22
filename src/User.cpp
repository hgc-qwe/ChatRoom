#include <iostream>
#include <string>
#include "User.h"

void User::setId(int id) {
    Id = id;
}
int User::getId() {
    return Id;
}

void User::setName(std::string name) {
    Name = name;
}
std::string User::getName() {
    return Name;
}

void User::setPassword(std::string password) {
    Password = password;
}
std::string User::getPassword() {
    return Password;
}

void User::setState(bool state) {
    State = state;
}
std::string User::getState() {
    return State;
}