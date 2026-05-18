#include "f4mp/Config.h"
#include "f4mp/Logger.h"
#include <fstream>
#include <filesystem>

namespace f4mp {

Config& Config::GetInstance() {
    static Config instance;
    return instance;
}

bool Config::Load(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_configPath = path;

    if (!std::filesystem::exists(path)) {
        Logger::Warn("Config file not found at '%s', creating default", path.c_str());

        m_data = nlohmann::json{
            {"server", {{"address", "127.0.0.1"}, {"port", 7779}}},
            {"player", {{"name", "Survivor"}}},
            {"debug", {{"logging", false}}},
            {"network", {{"interpolation_delay", 0.1f}, {"extrapolation_limit", 0.5f}}}
        };

        return Save(path);
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            Logger::Error("Failed to open config file: %s", path.c_str());
            return false;
        }

        file >> m_data;
        Logger::Info("Config loaded from %s", path.c_str());
        return true;
    } catch (const std::exception& e) {
        Logger::Error("Failed to parse config: %s", e.what());
        return false;
    }
}

bool Config::Save(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    try {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());

        std::ofstream file(path);
        if (!file.is_open()) {
            Logger::Error("Failed to create config file: %s", path.c_str());
            return false;
        }

        file << m_data.dump(4);
        Logger::Info("Config saved to %s", path.c_str());
        return true;
    } catch (const std::exception& e) {
        Logger::Error("Failed to save config: %s", e.what());
        return false;
    }
}

std::string Config::GetServerAddress() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        return m_data.at("server").at("address").get<std::string>();
    } catch (...) {
        return "127.0.0.1";
    }
}

void Config::SetServerAddress(const std::string& addr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data["server"]["address"] = addr;
}

uint16_t Config::GetServerPort() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        return m_data.at("server").at("port").get<uint16_t>();
    } catch (...) {
        return 7779;
    }
}

void Config::SetServerPort(uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data["server"]["port"] = port;
}

std::string Config::GetPlayerName() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        return m_data.at("player").at("name").get<std::string>();
    } catch (...) {
        return "Survivor";
    }
}

void Config::SetPlayerName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data["player"]["name"] = name;
}

bool Config::GetDebugLogging() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        return m_data.at("debug").at("logging").get<bool>();
    } catch (...) {
        return false;
    }
}

void Config::SetDebugLogging(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data["debug"]["logging"] = enable;
}

}
