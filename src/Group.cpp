#include <iostream>
#include "Group.h"

Group::Group(const std::string& name, const std::string desc): Name(name), Desc(desc) {};

void Group::setId(int id) {
        Id = id;
}

int Group::getId() {
    return Id;
}

void Group::setName(std::string name) {
    Name = name;
}
    
std::string Group::getName() {
    return Name;
}

void Group::setDesc(std::string desc) {
    Desc = desc;
}
    
std::string Group::getDesc() {
    return Desc;
}

std::vector<GroupUser> Group::getUsers() {
    return users;
}