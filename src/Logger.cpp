#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> Logger::logger = nullptr;

void Logger::init() {
    if (logger) return;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("server.log", true);

    logger = std::make_shared<spdlog::logger>("ChatServer", spdlog::sinks_init_list{console_sink, file_sink});
    spdlog::register_logger(logger);

    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::info);
}

std::shared_ptr<spdlog::logger> Logger::getLogger() {
    return logger;
}