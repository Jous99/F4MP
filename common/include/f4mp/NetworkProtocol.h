#pragma once

#include <string>
#include <cstdint>

namespace f4mp {

constexpr uint32_t PROTOCOL_VERSION_MAJOR = 1;
constexpr uint32_t PROTOCOL_VERSION_MINOR = 0;
constexpr uint32_t PROTOCOL_VERSION_PATCH = 0;

constexpr uint16_t DEFAULT_SERVER_PORT = 7779;
constexpr uint32_t MAX_PLAYERS = 64;
constexpr uint32_t TICK_RATE = 30;
constexpr float TICK_INTERVAL = 1.0f / static_cast<float>(TICK_RATE);

enum class MessageType : uint16_t {
    Invalid = 0,

    Connection = 100,
    ConnectionRequest,
    ConnectionAccepted,
    ConnectionRejected,
    Disconnect,
    Heartbeat,

    Player = 200,
    PlayerPosition,
    PlayerRotation,
    PlayerAnimation,
    PlayerInventory,
    PlayerStats,

    World = 300,
    EntitySpawn,
    EntityDespawn,
    EntityState,
    ObjectState,
    CellChange,

    Combat = 400,
    DamageDealt,
    DeathEvent,
    ProjectileFired,

    Chat = 500,
    ChatMessage,
    ChatBroadcast,

    Custom = 900,
    CustomEvent
};

#pragma pack(push, 1)

struct MessageHeader {
    MessageType type;
    uint32_t size;
    uint64_t timestamp;
    uint32_t sequence;
};

struct ConnectionRequestMsg {
    char playerName[64];
    uint32_t protocolVersion;
    uint64_t sessionId;
};

struct ConnectionAcceptedMsg {
    uint32_t playerId;
    uint64_t sessionId;
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
    bool isInCombat;
};

struct PlayerRotationMsg {
    uint32_t playerId;
    float yaw;
    float pitch;
    float roll;
};

struct PlayerAnimationMsg {
    uint32_t playerId;
    uint32_t animationId;
    float blendValue;
    bool isLooping;
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
    bool isVATS;
};

struct EntityStateMsg {
    uint32_t entityId;
    uint64_t formId;
    float x, y, z;
    float yaw, pitch, roll;
    uint8_t state;
    uint32_t health;
    uint32_t maxHealth;
};

#pragma pack(pop)

struct NetworkConfig {
    std::string serverAddress = "127.0.0.1";
    uint16_t serverPort = DEFAULT_SERVER_PORT;
    std::string playerName = "Survivor";
    bool enableDebugLogging = false;
    float interpolationDelay = 0.1f;
    float extrapolationLimit = 0.5f;
};

}
