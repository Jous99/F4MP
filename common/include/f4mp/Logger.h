#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <memory>

namespace f4mp {

class Logger {
public:
    static void Initialize(const std::string& logPath = "F4MP.log", bool debugMode = false);
    static void Shutdown();

    static std::shared_ptr<spdlog::logger> Get() { return s_logger; }

    static void Info(const char* fmt, ...);
    static void Warn(const char* fmt, ...);
    static void Error(const char* fmt, ...);
    static void Debug(const char* fmt, ...);
    static void Trace(const char* fmt, ...);

private:
    static std::shared_ptr<spdlog::logger> s_logger;
    static bool s_initialized;
};

}
