#pragma once

#include "f4mp/NetworkProtocol.h"

#include <steamnetworkingsockets/steamnetworkingsockets.h>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>

namespace f4mp {

struct ClientInfo {
    uint32_t id;
    HSteamNetConnection connection;
    std::string name;
    float x, y, z;
    float yaw, pitch, roll;
    uint32_t cellId;
    bool isConnected;
    double lastUpdate;
};

class Server {
public:
    static Server& GetInstance();

    bool Initialize(uint16_t port = DEFAULT_SERVER_PORT);
    void Shutdown();

    void Run();
    void Update();

    bool IsRunning() const { return m_running; }
    uint32_t GetPlayerCount() const;

private:
    Server() = default;

    void HandleConnectionRequest(HSteamNetConnection conn, const ConnectionRequestMsg* msg);
    void HandlePlayerPosition(HSteamNetConnection conn, const PlayerPositionMsg* msg);
    void HandleChatMessage(HSteamNetConnection conn, const ChatMessageMsg* msg);

    void BroadcastMessage(MessageType type, const void* data, uint32_t size, uint32_t excludeId = 0);
    void SendMessageToClient(uint32_t clientId, MessageType type, const void* data, uint32_t size);

    void ProcessMessages();
    void HandleConnectionStatusChanges();

    ISteamNetworkingSockets* m_sockets = nullptr;
    HSteamListenSocket m_listenSocket = k_HSteamListenSocket_Invalid;

    std::unordered_map<HSteamNetConnection, ClientInfo> m_clients;
    std::mutex m_clientsMutex;

    uint32_t m_nextClientId = 1;
    uint32_t m_maxPlayers = MAX_PLAYERS;

    bool m_running = false;
    bool m_initialized = false;
};

}
