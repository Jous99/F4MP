#pragma once

#include "f4mp/NetworkProtocol.h"

#include <steamnetworkingsockets/steamnetworkingsockets.h>
#include <unordered_map>
#include <mutex>
#include <string>

namespace f4mp {

class ClientManager {
public:
    static ClientManager& GetInstance();

    uint32_t AddClient(HSteamNetConnection conn, const std::string& name);
    void RemoveClient(HSteamNetConnection conn);

    ClientInfo* GetClient(HSteamNetConnection conn);
    ClientInfo* GetClientById(uint32_t id);

    uint32_t GetPlayerCount() const;
    bool IsFull() const;

    void ForEachClient(std::function<void(ClientInfo&)> callback);

private:
    ClientManager() = default;

    std::unordered_map<HSteamNetConnection, ClientInfo> m_clients;
    mutable std::mutex m_mutex;
    uint32_t m_nextId = 1;
};

}
