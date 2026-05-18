#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cassert>
#include <csignal>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "ThirdParty/nlohmann/json.hpp"
#include "Config.hpp"
#include "Network/GameServer.h"

#if defined(_WINDOWS)
#include <windows.h>
#include <libloaderapi.h>
#else
#include <unistd.h>
#endif

static volatile sig_atomic_t g_running = 1;

void SignalHandler(int signal) {
    g_running = 0;
    spdlog::info("[Server] Received signal {}, shutting down...", signal);
}

int UMain() {
    spdlog::info("[Server] F4MP Server v1.0.0 - Next-Gen Compatible");
    spdlog::info("[Server] Copyright (C) 2020-2026 Alin Octavian, Benjamin Kyd");
    spdlog::info("[Server] This program comes with ABSOLUTELY NO WARRANTY.");
    spdlog::info("[Server] This is free software, redistribute under certain conditions; see LICENSE");

    uint16_t port = static_cast<uint16_t>(Config::getInstance().Port);
    std::string ip = Config::getInstance().Ip;

    if (!Config::getInstance().LogLocation.empty() && Config::getInstance().LogLocation != "NONE") {
        auto file_logger = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger", Config::getInstance().LogLocation);
        spdlog::set_default_logger(file_logger);
    }

    if (!GameServer::GetInstance().Initialize(port)) {
        spdlog::error("[Server] Failed to initialize server on port {}", port);
        return 1;
    }

    spdlog::info("[Server] Server started on {}:{}", ip, port);
    spdlog::info("[Server] Press Ctrl+C to stop");

    GameServer::GetInstance().Run();

    GameServer::GetInstance().Shutdown();
    return 0;
}

#if defined(_WINDOWS)
#define SERVICE_NAME L"F4MPService"

static DWORD WINAPI serviceWorkerThread(LPVOID lpParam) {
    UMain();
    return 0;
}

static void WINAPI serviceMain(DWORD argc, TCHAR** argv) {
    // TODO: Implement Windows Service properly
    UMain();
}

#endif

int main(int argc, char** argv) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    auto console = spdlog::stdout_color_mt("console");
    auto err_logger = spdlog::stderr_color_mt("stderr");

    spdlog::get("console")->info("F4MP  Copyright (C) 2020  Alin Octavian, Benjamin Kyd \n"
                                 "This program comes with ABSOLUTELY NO WARRANTY. \n"
                                 "This is free software, and you are welcome to redistribute it \n"
                                 "\"under certain conditions; Read LICENSE for full details. \n \n");

    std::filesystem::path ConfigLocation{"./config.json"};

    if (!std::filesystem::exists(ConfigLocation)) {
        nlohmann::json Config;
        Config["ip"] = "127.0.0.1";
        Config["port"] = 7779;
        Config["run-as-service"] = false;
        Config["player-limit"] = 100;
        Config["log-location"] = "./logs.log";
        std::ofstream o{ConfigLocation};
        o << std::setw(4) << Config << std::endl;
        spdlog::get("stderr")->error("ERROR: no config exists. One has been created");
        exit(0);
    }

    std::ifstream InConfig{ConfigLocation};
    InConfig >> Config::getInstance().JSON;
    Config::getInstance().Setup();

    if (Config::getInstance().RunAsService == true) {
#if defined(_WINDOWS)
        spdlog::get("console")->info("Starting as Windows service");

        SERVICE_TABLE_ENTRY ServiceTable[] = {
            {reinterpret_cast<LPWSTR>(SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)serviceMain},
            {nullptr, nullptr}
        };

        if (StartServiceCtrlDispatcher(ServiceTable) == FALSE) {
            spdlog::get("console")->warn("Failed to start as service, running normally");
            return UMain();
        }
#else
        spdlog::get("console")->info("Starting as UNIX daemon");

        pid_t pid = fork();
        if (pid < 0) {
            spdlog::get("console")->error("Failed to fork, running normally");
            return UMain();
        }

        if (pid > 0) {
            spdlog::get("console")->info("Parent process terminating");
            exit(0);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        return UMain();
#endif
    }

    return UMain();
}
