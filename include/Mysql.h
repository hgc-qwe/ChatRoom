#include <mysql/mysql.h>
#include <string>

class Mysql {
private:
    MYSQL* conn = nullptr;

public:
    Mysql();

    bool connect();

    bool update(const std::string& sql);

    MYSQL_RES* query(const std::string& sql);

    ~Mysql();
};