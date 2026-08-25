#ifndef F4MPCLIENT_NETWORKCLIENT_H
#define F4MPCLIENT_NETWORKCLIENT_H

#include <cstdint>
#include <string>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <chrono>

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
    PlayerAppearance = 13,
    ChatMessage = 20,
    DamageDealt = 30,
    EntitySpawn = 40,
    EntityState = 41,
    NpcState = 50,      // el host difunde el estado de un NPC (por FormID)
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

// (tipo Disconnect) el servidor la envia cuando un jugador se va o es expulsado,
// para eliminar su cuerpo AL INSTANTE sin esperar al timeout de 5 s.
struct PlayerLeftMsg {
    uint32_t playerId;
};

struct PlayerPositionMsg {
    uint32_t playerId;
    float x, y, z;
    float velocityX, velocityY, velocityZ;
    float angleZ;        // heading de vista (rad): hacia donde mira el jugador
    float speed;         // velocidad de movimiento (unidades/seg)
    float moveDir;       // direccion del movimiento relativa al facing (rad, 0 = adelante)
    uint32_t cellId;
    bool isMoving;
    bool isRunning;
    bool isSprinting;
    bool isSneaking;
    bool isJumping;
    bool weaponDrawn;   // arma/magia en mano (postura de combate)
};

// Apariencia del jugador (Fase 2A): se envia una vez al conectar y se reparte.
struct PlayerAppearanceMsg {
    uint32_t playerId;
    uint32_t raceFormID;
    uint32_t hairColor;      // RGBA empaquetado (BGSColorForm::color)
    int32_t  sex;            // 0 male, 1 female, -1 none
    uint8_t  skinR, skinG, skinB, skinA;  // bodyTintColor del NPC
    uint8_t  numHeadParts;
    uint8_t  _pad[3];
    uint32_t headParts[16];  // formIDs de head parts (pelo, ojos, barba, cejas...)
};

struct ChatMessageMsg {
    uint32_t senderId;
    char message[256];
    uint8_t channel;
};

// Estado de un NPC que difunde el HOST. El FormID identifica al mismo NPC en las dos
// maquinas (para NPCs "fijos" del mundo, el FormID coincide). El cliente busca su NPC
// local por ese FormID y lo conduce a este estado.
struct NpcStateMsg {
    uint32_t formId;     // FormID del NPC (identidad comun entre maquinas)
    float x, y, z;
    float angleZ;        // heading
    float speed;         // para animacion (mas adelante)
    bool isMoving;
    bool isSneaking;
    bool isDead;         // para muerte (mas adelante)
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

    // Copia del estado de los jugadores remotos (id -> ultima posicion).
    std::unordered_map<uint32_t, PlayerPositionMsg> GetRemotePlayers();

    // Apariencia recibida de un jugador remoto. Devuelve false si aun no llego.
    bool GetAppearance(uint32_t playerId, PlayerAppearanceMsg& out);

    // Copia de los NPCs difundidos por el host (formId -> ultimo estado).
    std::unordered_map<uint32_t, NpcStateMsg> GetRemoteNpcs();

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

    // Jugadores remotos (actualizado desde PumpMessages).
    std::mutex m_remoteMutex;
    std::unordered_map<uint32_t, PlayerPositionMsg> m_remotePlayers;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> m_remoteLastSeen;
    std::unordered_map<uint32_t, PlayerAppearanceMsg> m_remoteAppearance;

    // NPCs difundidos por el host (formId -> estado) + cuando se vio cada uno.
    std::unordered_map<uint32_t, NpcStateMsg> m_remoteNpcs;
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> m_npcLastSeen;
};

}

#endif
