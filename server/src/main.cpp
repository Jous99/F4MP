#include "Server.h"
#include "f4mp/Logger.h"
#include "f4mp/Config.h"

#include <iostream>
#include <string>
#include <csignal>
#include <thread>

static volatile sig_atomic_t g_running = 1;

void SignalHandler(int signal) {
    g_running = 0;
    f4mp::Logger::Info("Received signal %d, shutting down...", signal);
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    f4mp::Logger::Initialize("F4MPServer.log", true);

    f4mp::Logger::Info("========================================");
    f4mp::Logger::Info("F4MP Server v1.0.0");
    f4mp::Logger::Info("Fallout 4 Multiplayer - Revived");
    f4mp::Logger::Info("========================================");

    f4mp::Config::GetInstance().Load("F4MPServer.json");

    uint16_t port = f4mp::Config::GetInstance().GetServerPort();

    if (!f4mp::Server::GetInstance().Initialize(port)) {
        f4mp::Logger::Error("Failed to initialize server");
        return 1;
    }

    f4mp::Logger::Info("Server started. Press Ctrl+C to stop.");

    while (g_running) {
        f4mp::Server::GetInstance().Update();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    f4mp::Server::GetInstance().Shutdown();
    f4mp::Logger::Shutdown();

    return 0;
}
