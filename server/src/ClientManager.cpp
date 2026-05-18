#include "ClientManager.h"
#include "f4mp/Logger.h"

namespace f4mp {

ClientManager& ClientManager::GetInstance() {
    static ClientManager instance;
    return instance;
}

uint32_t ClientManager::AddClient(HSteamNetConnection conn, const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ClientInfo client{};
    client.id = m_nextId++;
    client.connection = conn;
    client.name = name;
    client.isConnected = true;

    m_clients[conn] = client;

    Logger::Info("Client added: %s (ID: %d)", name.c_str(), client.id);
    return client.id;
}

void ClientManager::RemoveClient(HSteamNetConnection conn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clients.erase(conn);
}

ClientInfo* ClientManager::GetClient(HSteamNetConnection conn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_clients.find(conn);
    return (it != m_clients.end()) ? &it->second : nullptr;
}

ClientInfo* ClientManager::GetClientById(uint32_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [conn, client] : m_clients) {
        if (client.id == id) {
            return &client;
        }
    }
    return nullptr;
}

uint32_t ClientManager::GetPlayerCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t count = 0;
    for (const auto& [conn, client] : m_clients) {
        if (client.isConnected) count++;
    }
    return count;
}

bool ClientManager::IsFull() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_clients.size() >= MAX_PLAYERS;
}

void ClientManager::ForEachClient(std::function<void(ClientInfo&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [conn, client] : m_clients) {
        callback(client);
    }
}

}
