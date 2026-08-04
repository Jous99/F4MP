#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cassert>
#include <csignal>
#include <thread>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

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
    spdlog::info("F4MP Server v1.0.0 - Next-Gen Compatible");

    uint16_t port = static_cast<uint16_t>(Config::getInstance().Port);

    if (!Config::getInstance().LogLocation.empty() && Config::getInstance().LogLocation != "NONE") {
        auto file_logger = spdlog::basic_logger_mt<spdlog::async_factory>("file_logger", Config::getInstance().LogLocation);
        spdlog::set_default_logger(file_logger);
    }

    GameServer::GetInstance().SetName(Config::getInstance().Name);
    GameServer::GetInstance().SetMaxPlayers(Config::getInstance().PlayerLimit);

    if (!GameServer::GetInstance().Initialize(port)) {
        spdlog::error("[Server] Failed to initialize server on port {}", port);
        return 1;
    }

    // Hilo que lee comandos del teclado y los encola para el bucle principal.
    std::thread([]() {
        std::string line;
        while (std::getline(std::cin, line)) {
            GameServer::GetInstance().QueueCommand(line);
        }
    }).detach();

    GameServer::GetInstance().Run();

    GameServer::GetInstance().Shutdown();
    return 0;
}

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
        Config["server-name"] = "My F4MP Server";
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

    // Modo servicio de Windows: pendiente. Por ahora se ejecuta en modo consola.
    if (Config::getInstance().RunAsService == true) {
        spdlog::get("console")->warn("run-as-service todavia no esta implementado; ejecutando en modo consola");
    }

    return UMain();
}
