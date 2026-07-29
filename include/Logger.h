#pragma once
#include <memory>
#include <spdlog/spdlog.h>

class Logger {
public:
    static void init();
    static std::shared_ptr<spdlog::logger> getLogger();
private:
    static std::shared_ptr<spdlog::logger> logger;
};

#define LOG_INFO(...) \
    Logger::getLogger()->info(__VA_ARGS__)


#define LOG_ERROR(...) \
    Logger::getLogger()->error(__VA_ARGS__)


#define LOG_WARN(...) \
    Logger::getLogger()->warn(__VA_ARGS__)
    