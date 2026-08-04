// F4MP Simulator
// ---------------
// Cliente de prueba: se conecta al servidor F4MP como si fuera un jugador,
// hace el handshake, manda posiciones falsas y muestra el chat. Sirve para
// testear el servidor SIN Fallout. Puede simular varios jugadores a la vez.
//
//   F4MPSim [host] [puerto] [numero_de_bots]
//   Ejemplo:  F4MPSim 127.0.0.1 7779 3

#define _CRT_SECURE_NO_WARNINGS

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <csignal>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <chrono>

// ---- Protocolo (igual que el del cliente/servidor) ----
enum class MessageType : uint16_t {
    Invalid = 0, ConnectionRequest = 1, ConnectionAccepted = 2, ConnectionRejected = 3,
    Disconnect = 4, PlayerPosition = 10, ChatMessage = 20,
};

#pragma pack(push, 1)
struct MessageHeader { MessageType type; uint32_t size; uint64_t timestamp; };
struct ConnectionRequestMsg { char playerName[64]; uint32_t protocolVersion; };
struct ConnectionAcceptedMsg { uint32_t playerId; uint32_t currentPlayers; uint32_t maxPlayers; };
struct PlayerPositionMsg {
    uint32_t playerId; float x, y, z; float velocityX, velocityY, velocityZ;
    uint32_t cellId; bool isRunning; bool isSneaking;
};
struct ChatMessageMsg { uint32_t senderId; char message[256]; uint8_t channel; };
#pragma pack(pop)

constexpr uint32_t PROTOCOL_VERSION = 1;

// ---- Estado global ----
static ISteamNetworkingSockets* g_sockets = nullptr;
static std::unordered_map<HSteamNetConnection, std::string> g_names;
static volatile std::sig_atomic_t g_running = 1;

static void OnSignal(int) { g_running = 0; }

static void SendPacket(HSteamNetConnection conn, MessageType type, const void* data, uint32_t size) {
    MessageHeader h;
    h.type = type;
    h.size = sizeof(MessageHeader) + size;
    h.timestamp = 0;
    std::vector<uint8_t> buf(h.size);
    memcpy(buf.data(), &h, sizeof(h));
    if (data && size) memcpy(buf.data() + sizeof(h), data, size);
    g_sockets->SendMessageToConnection(conn, buf.data(), (uint32_t)buf.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);
}

static void OnConnStatusChanged(SteamNetConnectionStatusChangedCallback_t* info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connected: {
            std::string name = g_names.count(info->m_hConn) ? g_names[info->m_hConn] : "Bot";
            ConnectionRequestMsg req{};
            strncpy(req.playerName, name.c_str(), sizeof(req.playerName) - 1);
            req.protocolVersion = PROTOCOL_VERSION;
            SendPacket(info->m_hConn, MessageType::ConnectionRequest, &req, sizeof(req));
            spdlog::info("[{}] transporte conectado, handshake enviado", name);
            break;
        }
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            spdlog::warn("Conexion cerrada: {}", info->m_info.m_szEndDebug);
            g_sockets->CloseConnection(info->m_hConn, 0, nullptr, false);
            break;
        default:
            break;
    }
}

int main(int argc, char** argv) {
    std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    uint16_t    port = (argc > 2) ? (uint16_t)atoi(argv[2]) : 7779;
    int         count = (argc > 3) ? atoi(argv[3]) : 1;
    if (count < 1) count = 1;

    std::signal(SIGINT, OnSignal);

    SteamNetworkingErrMsg err;
    if (!GameNetworkingSockets_Init(nullptr, err)) {
        spdlog::error("No se pudo iniciar GameNetworkingSockets: {}", err);
        return 1;
    }
    g_sockets = SteamNetworkingSockets();
    SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);
    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(&OnConnStatusChanged);

    char addrStr[64];
    snprintf(addrStr, sizeof(addrStr), "%s:%d", host.c_str(), port);
    SteamNetworkingIPAddr addr;
    addr.Clear();
    if (!addr.ParseString(addrStr)) {
        spdlog::error("Direccion invalida: {}", addrStr);
        return 1;
    }

    std::vector<HSteamNetConnection> conns;
    for (int i = 0; i < count; ++i) {
        HSteamNetConnection c = g_sockets->ConnectByIPAddress(addr, 0, nullptr);
        std::string name = (count == 1) ? "TestBot" : ("Bot" + std::to_string(i + 1));
        g_names[c] = name;
        conns.push_back(c);
    }
    spdlog::info("Simulando {} jugador(es) contra {}:{}. Pulsa Ctrl+C para salir.", count, host, port);

    auto lastPos = std::chrono::steady_clock::now();
    float t = 0.0f;

    while (g_running) {
        g_sockets->RunCallbacks();

        // Recibir mensajes de cada bot
        for (auto c : conns) {
            SteamNetworkingMessage_t* msgs[16];
            int n = g_sockets->ReceiveMessagesOnConnection(c, msgs, 16);
            for (int i = 0; i < n; ++i) {
                SteamNetworkingMessage_t* msg = msgs[i];
                if (msg->m_cbSize >= (int)sizeof(MessageHeader)) {
                    const auto* h = (const MessageHeader*)msg->m_pData;
                    const void* p = (const uint8_t*)msg->m_pData + sizeof(MessageHeader);
                    if (h->type == MessageType::ConnectionAccepted) {
                        const auto* a = (const ConnectionAcceptedMsg*)p;
                        spdlog::info("[{}] aceptado por el servidor. ID {} - jugadores {}/{}",
                            g_names[c], a->playerId, a->currentPlayers, a->maxPlayers);
                    } else if (h->type == MessageType::ChatMessage) {
                        const auto* ch = (const ChatMessageMsg*)p;
                        spdlog::info("[chat] {}", ch->message);
                    }
                }
                msg->Release();
            }
        }

        // Cada segundo, cada bot manda una posicion (moviendose en circulo)
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(now - lastPos).count() >= 1.0f) {
            t += 1.0f;
            lastPos = now;
            for (auto c : conns) {
                PlayerPositionMsg pos{};
                pos.x = std::sin(t) * 100.0f;
                pos.y = std::cos(t) * 100.0f;
                pos.z = 0.0f;
                pos.cellId = 1;
                SendPacket(c, MessageType::PlayerPosition, &pos, sizeof(pos));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    spdlog::info("Cerrando conexiones...");
    for (auto c : conns) g_sockets->CloseConnection(c, 0, "bye", false);
    g_sockets->RunCallbacks();
    GameNetworkingSockets_Kill();
    spdlog::info("Simulador detenido.");
    return 0;
}
