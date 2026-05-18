#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <mutex>

namespace f4mp {

class Config {
public:
    static Config& GetInstance();

    bool Load(const std::string& path = "Data/F4SE/Plugins/F4MP.json");
    bool Save(const std::string& path = "Data/F4SE/Plugins/F4MP.json") const;

    std::string GetServerAddress() const;
    void SetServerAddress(const std::string& addr);

    uint16_t GetServerPort() const;
    void SetServerPort(uint16_t port);

    std::string GetPlayerName() const;
    void SetPlayerName(const std::string& name);

    bool GetDebugLogging() const;
    void SetDebugLogging(bool enable);

private:
    Config() = default;

    mutable std::mutex m_mutex;
    nlohmann::json m_data;
    std::string m_configPath;
};

}
