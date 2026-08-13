#pragma one
#include <string>
#include <random>

class VerifyCode {
public:
    static std::string generate() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<int> dist(0, 9);

        std::string code;
        for (int i = 0; i < 4; i++) {
            code += char('0' + dist(gen));
        }
        return code;
    }
};