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

class NetworkClient {
public:
    static NetworkClient& GetInstance();

    bool Initialize();
    void Shutdown();

    bool Connect(const std::string& address, uint16_t port = DEFAULT_SERVER_PORT);
    void Disconnect();

    bool IsConnected() const { return m_connected; }

    void SetPlayerName(const std::string& name) { m_playerName = name; }

    // OJO: no se llama SendMessage a proposito: en Windows <windows.h>
    // define SendMessage como una macro y romperia la compilacion.
    void SendPacket(MessageType type, const void* data, uint32_t size);

    void SetMessageCallback(std::function<void(const MessageHeader&, const void*)> callback);
    void SetConnectionCallback(std::function<void(bool connected)> callback);

    // Llamar cada frame: procesa callbacks de GNS y mensajes entrantes.
    void PumpMessages();

    uint32_t GetPlayerId() const { return m_playerId; }

private:
    NetworkClient() = default;

    // Trampolin estatico para el callback global de GNS -> instancia singleton.
    static void OnConnStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* info);
    void OnConnStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

    void SendConnectionRequest();

    ISteamNetworkingSockets* m_sockets = nullptr;
    ISteamNetworkingUtils* m_utils = nullptr;

    HSteamNetConnection m_connection = k_HSteamNetConnection_Invalid;

    bool m_connected = false;      // true cuando el servidor acepta (ConnectionAccepted)
    bool m_initialized = false;
    uint32_t m_playerId = 0;
    std::string m_playerName = "Wastelander";

    std::mutex m_mutex;

    std::function<void(const MessageHeader&, const void*)> m_messageCallback;
    std::function<void(bool)> m_connectionCallback;
};

}

#endif
