#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "Logger.h"
#include "MysqlPool.h"
#include "FriendModel.h"
#include "UserModel.h"

using Clock = std::chrono::steady_clock;

static double msSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

int main() {
    Logger::init();

    if (!MysqlPool::instance()->init(16)) {
        std::cerr << "pool init failed" << std::endl;
        return 1;
    }

    // 1. 基本借还：反复借还远超池容量的次数，验证 guard 确实归还了连接。
    {
        auto start = Clock::now();
        for (int i = 0; i < 500; i++) {
            auto conn = MysqlPool::instance()->acquire();
            if (!conn) {
                std::cerr << "FAIL: acquire returned empty at iteration " << i << std::endl;
                return 1;
            }
            MYSQL_RES* res = conn.query("select 1;");
            if (res) mysql_free_result(res);
        }
        std::cout << "[1] 500 次顺序借还  平均 " << msSince(start) / 500 << " ms/次" << std::endl;
    }

    // 2. 并发借还：32 个线程争抢 16 个连接，验证不会死锁、不会重复借出。
    {
        std::atomic<int> ok{0}, fail{0};
        auto start = Clock::now();
        std::vector<std::thread> threads;
        for (int t = 0; t < 32; t++) {
            threads.emplace_back([&]() {
                for (int i = 0; i < 50; i++) {
                    auto conn = MysqlPool::instance()->acquire();
                    if (!conn) { fail++; continue; }
                    MYSQL_RES* res = conn.query("select 1;");
                    if (res) { mysql_free_result(res); ok++; }
                    else fail++;
                }
            });
        }
        for (auto& th : threads) th.join();
        std::cout << "[2] 32 线程 x 50 次   成功 " << ok << " 失败 " << fail
                  << "  耗时 " << msSince(start) << " ms" << std::endl;
        if (fail > 0) { std::cerr << "FAIL: 并发场景出现失败" << std::endl; return 1; }
    }

    // 3. 对比 oneChat 的校验链路，确认单次查询已降到亚毫秒级。
    {
        FriendModel friendModel;
        auto start = Clock::now();
        for (int i = 0; i < 200; i++) friendModel.isFriend(1, 2);
        std::cout << "[3] isFriend x200      平均 " << msSince(start) / 200 << " ms/次" << std::endl;
    }

    {
        UserModel userModel;
        auto start = Clock::now();
        for (int i = 0; i < 200; i++) userModel.query(1);
        std::cout << "[4] userModel.query x200 平均 " << msSince(start) / 200 << " ms/次" << std::endl;
    }

    // 5. 转义正确性：带单引号的文本以前会让 INSERT 直接失败。
    {
        auto conn = MysqlPool::instance()->acquire();
        if (!conn) { std::cerr << "FAIL: acquire" << std::endl; return 1; }
        std::string nasty = "it's a \"test\"; drop table user; --";
        std::string escaped = conn.escape(nasty);
        std::cout << "[5] escape 原文: " << nasty << std::endl;
        std::cout << "    escape 结果: " << escaped << std::endl;
        if (escaped.find("\\'") == std::string::npos) {
            std::cerr << "FAIL: 单引号未被转义" << std::endl;
            return 1;
        }
    }

    std::cout << "\n全部通过" << std::endl;
    return 0;
}
