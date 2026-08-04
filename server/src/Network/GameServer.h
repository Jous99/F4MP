#ifndef F4MPSERVER_GAMESERVER_H
#define F4MPSERVER_GAMESERVER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

enum class MessageType : uint16_t {
    Invalid = 0,
    ConnectionRequest = 1,
    ConnectionAccepted = 2,
    ConnectionRejected = 3,
    Disconnect = 4,
    PlayerPosition = 10,
    PlayerRotation = 11,
    PlayerAnimation = 12,
    ChatMessage = 20,
    DamageDealt = 30,
    EntitySpawn = 40,
    EntityState = 41,
};

#pragma pack(push, 1)

struct MessageHeader {
    MessageType type;
    uint32_t size;
    uint64_t timestamp;
};

struct ConnectionRequestMsg {
    char playerName[64];
    uint32_t protocolVersion;
};

struct ConnectionAcceptedMsg {
    uint32_t playerId;
    uint32_t currentPlayers;
    uint32_t maxPlayers;
};

struct PlayerPositionMsg {
    uint32_t playerId;
    float x, y, z;
    float velocityX, velocityY, velocityZ;
    uint32_t cellId;
    bool isRunning;
    bool isSneaking;
};

struct ChatMessageMsg {
    uint32_t senderId;
    char message[256];
    uint8_t channel;
};

struct DamageDealtMsg {
    uint32_t attackerId;
    uint32_t targetId;
    uint64_t targetFormId;
    float damageAmount;
    uint8_t damageType;
    bool isCritical;
};

#pragma pack(pop)

constexpr uint32_t PROTOCOL_VERSION = 1;
constexpr uint16_t DEFAULT_SERVER_PORT = 7779;
constexpr uint32_t MAX_PLAYERS = 64;

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

class GameServer {
public:
    static GameServer& GetInstance();

    bool Initialize(uint16_t port = DEFAULT_SERVER_PORT);
    void Shutdown();

    void Run();
    void Update();

    bool IsRunning() const { return m_running; }
    uint32_t GetPlayerCount() const;

    void SetMaxPlayers(uint32_t n) { if (n > 0) m_maxPlayers = n; }

private:
    GameServer() = default;

    void HandleConnectionRequest(HSteamNetConnection conn, const ConnectionRequestMsg* msg);
    void HandlePlayerPosition(HSteamNetConnection conn, const PlayerPositionMsg* msg);
    void HandleChatMessage(HSteamNetConnection conn, const ChatMessageMsg* msg);

    void BroadcastMessage(MessageType type, const void* data, uint32_t size, uint32_t excludeId = 0);
    void SendMessageToClient(uint32_t clientId, MessageType type, const void* data, uint32_t size);

    void ProcessMessages();

    // Callback global de GNS -> instancia singleton.
    static void OnConnStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* info);
    void OnConnStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

    ISteamNetworkingSockets* m_sockets = nullptr;
    HSteamListenSocket m_listenSocket = k_HSteamListenSocket_Invalid;
    HSteamNetPollGroup m_pollGroup = k_HSteamNetPollGroup_Invalid;

    std::unordered_map<HSteamNetConnection, ClientInfo> m_clients;
    std::mutex m_clientsMutex;

    uint32_t m_nextClientId = 1;
    uint32_t m_maxPlayers = MAX_PLAYERS;

    bool m_running = false;
    bool m_initialized = false;
};

#endif
