#include <iostream>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;

int main() {
    json j;

    j["type"] = "chat";
    j["msg"] = "Hello";

    string s = j.dump();

    cout << s << endl;

    json x = json::parse(s);

    cout << x["type"] << endl;
    cout << x["msg"] << endl;
}