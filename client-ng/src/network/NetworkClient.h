#ifndef F4MPCLIENT_NETWORKCLIENT_H
#define F4MPCLIENT_NETWORKCLIENT_H

#include <cstdint>
#include <string>
#include <functional>
#include <mutex>

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

namespace Network {

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

#pragma pack(pop)

constexpr uint32_t PROTOCOL_VERSION = 1;
constexpr uint16_t DEFAULT_SERVER_PORT = 7779;

class NetworkClient {
public:
    static NetworkClient& GetInstance();

    bool Initialize();
    void Shutdown();

    bool Connect(const std::string& address, uint16_t port = DEFAULT_SERVER_PORT);
    void Disconnect();

    bool IsConnected() const { return m_connected; }

    void SetPlayerName(const std::string& name) { m_playerName = name; }

    void SendPacket(MessageType type, const void* data, uint32_t size);

    // Llamar periodicamente: procesa callbacks de GNS y mensajes entrantes.
    void PumpMessages();

    uint32_t GetPlayerId() const { return m_playerId; }

private:
    NetworkClient() = default;

    static void OnConnStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* info);
    void OnConnStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);
    void SendConnectionRequest();

    ISteamNetworkingSockets* m_sockets = nullptr;
    ISteamNetworkingUtils* m_utils = nullptr;

    HSteamNetConnection m_connection = k_HSteamNetConnection_Invalid;

    bool m_connected = false;
    bool m_initialized = false;
    uint32_t m_playerId = 0;
    std::string m_playerName = "Wastelander";

    std::mutex m_mutex;
};

}

#endif
