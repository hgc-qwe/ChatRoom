#pragma once
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include "Mysql.h"

class MysqlGuard {
public:
    MysqlGuard(std::unique_ptr<Mysql> conn);
    ~MysqlGuard();
    MysqlGuard(const MysqlGuard&) = delete;
    MysqlGuard& operator=(const MysqlGuard&) = delete;
    MysqlGuard(MysqlGuard&& other) noexcept;
    explicit operator bool() const;
    Mysql* operator->();
    Mysql& operator*();
    MYSQL_RES* query(const std::string& sql);
    bool update(const std::string& sql);
    MYSQL* getcon();
    std::string escape(const std::string& str);
private:
    std::unique_ptr<Mysql> conn;
};

class MysqlPool {
public:
    static MysqlPool* instance();

    bool init(size_t size = 8);

    MysqlGuard acquire(int timeoutMs = 3000);

    void release(std::unique_ptr<Mysql> conn);

private:
    MysqlPool() = default;
    ~MysqlPool() = default;
    MysqlPool(const MysqlPool&) = delete;
    MysqlPool& operator=(const MysqlPool&) = delete;

    std::queue<std::unique_ptr<Mysql>> conns;
    std::mutex mutex;
    std::condition_variable cond;
    size_t poolSize{0};
    bool initialized{false};
};