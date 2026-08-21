#include <chrono>
#include "MysqlPool.h"
#include "Logger.h"

MysqlGuard::MysqlGuard(std::unique_ptr<Mysql> conn) : conn(std::move(conn)) {}

MysqlGuard::MysqlGuard(MysqlGuard&& other) noexcept : conn(std::move(other.conn)) {}

MysqlGuard::~MysqlGuard() {
    if (conn) MysqlPool::instance()->release(std::move(conn));
}

MysqlGuard::operator bool() const {
    return conn != nullptr;
}

Mysql* MysqlGuard::operator->() {
    return conn.get();
}

Mysql& MysqlGuard::operator*() {
    return *conn;
}

MYSQL_RES* MysqlGuard::query(const std::string& sql) {
    return conn ? conn->query(sql) : nullptr;
}

bool MysqlGuard::update(const std::string& sql) {
    return conn ? conn->update(sql) : false;
}

MYSQL* MysqlGuard::getcon() {
    return conn ? conn->getcon() : nullptr;
}

std::string MysqlGuard::escape(const std::string& str) {
    return conn ? conn->escape(str) : std::string();
}

MysqlPool* MysqlPool::instance() {
    static MysqlPool pool;
    return &pool;
}

bool MysqlPool::init(size_t size) {
    std::lock_guard<std::mutex> lock(mutex);
    if (initialized) return true;

    for (size_t i = 0; i < size; i++) {
        auto conn = std::make_unique<Mysql>();
        if (!conn->connect()) {
            LOG_ERROR("MysqlPool init failed at connection {}", i);
            return false;
        }
        conns.push(std::move(conn));
    }

    poolSize = size;
    initialized = true;
    LOG_INFO("MysqlPool initialized with {} connections", size);
    return true;
}

MysqlGuard MysqlPool::acquire(int timeoutMs) {
    std::unique_lock<std::mutex> lock(mutex);

    if (!initialized) {
        LOG_ERROR("MysqlPool not initialized");
        return MysqlGuard(nullptr);
    }

    if (!cond.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() { return !conns.empty(); })) {
        LOG_ERROR("MysqlPool acquire timeout, all {} connections busy", poolSize);
        return MysqlGuard(nullptr);
    }

    auto conn = std::move(conns.front());
    conns.pop();
    lock.unlock();

    if (!conn->ping()) {
        LOG_WARN("pooled connection is dead, reconnecting");
        auto fresh = std::make_unique<Mysql>();
        if (!fresh->connect()) {
            LOG_ERROR("reconnect failed, returning dead connection to pool");
            release(std::move(conn));
            return MysqlGuard(nullptr);
        }
        conn = std::move(fresh);
    }

    return MysqlGuard(std::move(conn));
}

void MysqlPool::release(std::unique_ptr<Mysql> conn) {
    if (!conn) return;
    {
        std::lock_guard<std::mutex> lock(mutex);
        conns.push(std::move(conn));
    }
    cond.notify_one();
}
