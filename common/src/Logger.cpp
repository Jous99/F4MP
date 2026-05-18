#include "f4mp/Logger.h"
#include <cstdarg>
#include <cstdio>

namespace f4mp {

std::shared_ptr<spdlog::logger> Logger::s_logger;
bool Logger::s_initialized = false;

void Logger::Initialize(const std::string& logPath, bool debugMode) {
    if (s_initialized) {
        Shutdown();
    }

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%H:%M:%S.%e] [%^%l%$] [F4MP] %v");

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath, true);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [F4MP] %v");

    s_logger = std::make_shared<spdlog::logger>("f4mp", spdlog::sinks_init_list{consoleSink, fileSink});

    if (debugMode) {
        s_logger->set_level(spdlog::level::trace);
    } else {
        s_logger->set_level(spdlog::level::info);
    }

    s_logger->flush_on(spdlog::level::info);
    spdlog::register_logger(s_logger);
    s_initialized = true;

    Info("Logger initialized");
}

void Logger::Shutdown() {
    if (s_logger) {
        s_logger->flush();
        spdlog::drop("f4mp");
        s_logger.reset();
        s_initialized = false;
    }
}

void Logger::Info(const char* fmt, ...) {
    if (!s_logger) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    s_logger->info(buf);
}

void Logger::Warn(const char* fmt, ...) {
    if (!s_logger) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    s_logger->warn(buf);
}

void Logger::Error(const char* fmt, ...) {
    if (!s_logger) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    s_logger->error(buf);
}

void Logger::Debug(const char* fmt, ...) {
    if (!s_logger) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    s_logger->debug(buf);
}

void Logger::Trace(const char* fmt, ...) {
    if (!s_logger) return;
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    s_logger->trace(buf);
}

}
