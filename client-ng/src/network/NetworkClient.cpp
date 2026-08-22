#include "network/NetworkClient.h"

#include <chrono>
#include <cstring>
#include <vector>

namespace Network {

NetworkClient& NetworkClient::GetInstance() {
    static NetworkClient instance;
    return instance;
}

bool NetworkClient::Initialize() {
    if (m_initialized) return true;

    REX::INFO("[Network] Inicializando GameNetworkingSockets...");

    SteamNetworkingErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
        REX::ERROR("[Network] Fallo al iniciar GNS: {}", errMsg);
        return false;
    }

    m_sockets = SteamNetworkingSockets();
    m_utils = SteamNetworkingUtils();
    if (!m_sockets || !m_utils) {
        REX::ERROR("[Network] No se pudieron obtener las interfaces de red");
        return false;
    }

    m_utils->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);
    m_utils->SetGlobalCallback_SteamNetConnectionStatusChanged(&NetworkClient::OnConnStatusChangedStatic);

    m_initialized = true;
    REX::INFO("[Network] GameNetworkingSockets inicializado");
    return true;
}

void NetworkClient::Shutdown() {
    if (m_connection != k_HSteamNetConnection_Invalid) {
        Disconnect();
    }
    if (m_initialized) {
        GameNetworkingSockets_Kill();
        m_initialized = false;
    }
}

bool NetworkClient::Connect(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized) {
        REX::WARN("[Network] La red no esta inicializada");
        return false;
    }
    if (m_connection != k_HSteamNetConnection_Invalid) {
        REX::WARN("[Network] Ya hay una conexion en curso");
        return false;
    }

    char addrStr[64];
    snprintf(addrStr, sizeof(addrStr), "%s:%d", address.c_str(), port);

    SteamNetworkingIPAddr serverAddr;
    serverAddr.Clear();
    if (!serverAddr.ParseString(addrStr)) {
        REX::ERROR("[Network] Direccion invalida: {}", addrStr);
        return false;
    }

    m_connection = m_sockets->ConnectByIPAddress(serverAddr, 0, nullptr);
    if (m_connection == k_HSteamNetConnection_Invalid) {
        REX::ERROR("[Network] No se pudo crear la conexion a {}", addrStr);
        return false;
    }

    REX::INFO("[Network] Conectando a {}...", addrStr);
    return true;
}

void NetworkClient::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connection != k_HSteamNetConnection_Invalid) {
        m_sockets->CloseConnection(m_connection, 0, "Disconnecting", false);
        m_connection = k_HSteamNetConnection_Invalid;
    }
    m_connected = false;
    m_playerId = 0;
    {
        std::lock_guard<std::mutex> rlock(m_remoteMutex);
        m_remotePlayers.clear();   // olvidar a los remotos: si no, quedan fantasmas
        m_remoteLastSeen.clear();
        m_remoteAppearance.clear();
    }
    REX::INFO("[Network] Desconectado");
}

void NetworkClient::SendPacket(MessageType type, const void* data, uint32_t size) {
    if (m_connection == k_HSteamNetConnection_Invalid) return;

    MessageHeader header;
    header.type = type;
    header.size = sizeof(MessageHeader) + size;
    header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    std::vector<uint8_t> buffer(header.size);
    memcpy(buffer.data(), &header, sizeof(MessageHeader));
    if (data && size > 0) {
        memcpy(buffer.data() + sizeof(MessageHeader), data, size);
    }

    m_sockets->SendMessageToConnection(m_connection, buffer.data(), (uint32_t)buffer.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);
}

void NetworkClient::SendConnectionRequest() {
    ConnectionRequestMsg req{};
    strncpy(req.playerName, m_playerName.c_str(), sizeof(req.playerName) - 1);
    req.protocolVersion = PROTOCOL_VERSION;
    SendPacket(MessageType::ConnectionRequest, &req, sizeof(req));
    REX::INFO("[Network] Handshake enviado como '{}'", m_playerName);
}

void NetworkClient::OnConnStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* info) {
    GetInstance().OnConnStatusChanged(info);
}

void NetworkClient::OnConnStatusChanged(SteamNetConnectionStatusChangedCallback_t* info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connected:
            REX::INFO("[Network] Transporte conectado, enviando handshake...");
            SendConnectionRequest();
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            REX::WARN("[Network] Conexion cerrada: {}", info->m_info.m_szEndDebug);
            if (m_connection != k_HSteamNetConnection_Invalid) {
                m_sockets->CloseConnection(m_connection, 0, nullptr, false);
                m_connection = k_HSteamNetConnection_Invalid;
            }
            m_connected = false;
            m_playerId = 0;
            {
                std::lock_guard<std::mutex> rlock(m_remoteMutex);
                m_remotePlayers.clear();   // olvidar a los remotos al cerrarse la conexion
                m_remoteLastSeen.clear();
                m_remoteAppearance.clear();
            }
            break;

        default:
            break;
    }
}

void NetworkClient::PumpMessages() {
    if (!m_initialized) return;

    m_sockets->RunCallbacks();

    if (m_connection == k_HSteamNetConnection_Invalid) return;

    SteamNetworkingMessage_t* msgs[16];
    int msgCount = m_sockets->ReceiveMessagesOnConnection(m_connection, msgs, 16);
    for (int i = 0; i < msgCount; ++i) {
        SteamNetworkingMessage_t* msg = msgs[i];
        if (msg->m_cbSize >= (int)sizeof(MessageHeader)) {
            const auto* header = reinterpret_cast<const MessageHeader*>(msg->m_pData);
            const void* payload = reinterpret_cast<const uint8_t*>(msg->m_pData) + sizeof(MessageHeader);

            if (header->type == MessageType::ConnectionAccepted) {
                const auto* accept = reinterpret_cast<const ConnectionAcceptedMsg*>(payload);
                m_playerId = accept->playerId;
                m_connected = true;
                REX::INFO("[Network] Conectado! ID {}, jugadores {}/{}",
                    m_playerId, accept->currentPlayers, accept->maxPlayers);
            } else if (header->type == MessageType::ConnectionRejected) {
                REX::ERROR("[Network] Conexion rechazada por el servidor");
            } else if (header->type == MessageType::PlayerPosition) {
                // Blindaje: solo procesar si el paquete trae el struct COMPLETO.
                // Un cliente en version vieja manda menos bytes -> lo ignoramos
                // (mejor que leer basura desalineada).
                const size_t payloadSize = (size_t)msg->m_cbSize - sizeof(MessageHeader);
                if (payloadSize < sizeof(PlayerPositionMsg)) {
                    static bool warned = false;
                    if (!warned) { warned = true;
                        REX::WARN("[Network] Paquete de posicion antiguo ({} < {} bytes): cliente remoto en version vieja, actualizalo",
                            payloadSize, sizeof(PlayerPositionMsg)); }
                    msg->Release();
                    continue;
                }
                const auto* pos = reinterpret_cast<const PlayerPositionMsg*>(payload);
                if (pos->playerId != m_playerId) {  // ignorar la nuestra
                    std::lock_guard<std::mutex> lock(m_remoteMutex);
                    bool nuevo = m_remotePlayers.find(pos->playerId) == m_remotePlayers.end();
                    m_remotePlayers[pos->playerId] = *pos;
                    m_remoteLastSeen[pos->playerId] = std::chrono::steady_clock::now();
                    if (nuevo) {
                        REX::INFO("[Network] Nuevo jugador remoto {} en x={:.0f} y={:.0f} z={:.0f}",
                            pos->playerId, pos->x, pos->y, pos->z);
                    }
                }
            } else if (header->type == MessageType::PlayerAppearance) {
                const size_t payloadSize = (size_t)msg->m_cbSize - sizeof(MessageHeader);
                if (payloadSize >= sizeof(PlayerAppearanceMsg)) {
                    const auto* ap = reinterpret_cast<const PlayerAppearanceMsg*>(payload);
                    if (ap->playerId != m_playerId) {
                        std::lock_guard<std::mutex> lock(m_remoteMutex);
                        m_remoteAppearance[ap->playerId] = *ap;
                        REX::INFO("[Network] Apariencia de jugador {}: sexo={} raza={:#x} pelo={:#x} headparts={}",
                            ap->playerId, ap->sex, ap->raceFormID, ap->hairColor, ap->numHeadParts);
                    }
                }
            }
        }
        msg->Release();
    }
}

bool NetworkClient::GetAppearance(uint32_t playerId, PlayerAppearanceMsg& out) {
    std::lock_guard<std::mutex> lock(m_remoteMutex);
    auto it = m_remoteAppearance.find(playerId);
    if (it == m_remoteAppearance.end()) return false;
    out = it->second;
    return true;
}

std::unordered_map<uint32_t, PlayerPositionMsg> NetworkClient::GetRemotePlayers() {
    std::lock_guard<std::mutex> lock(m_remoteMutex);
    // Olvidar remotos que llevan >5 s sin mandar nada (se cayeron sin avisar).
    const auto ahora = std::chrono::steady_clock::now();
    for (auto it = m_remotePlayers.begin(); it != m_remotePlayers.end();) {
        auto seen = m_remoteLastSeen.find(it->first);
        if (seen != m_remoteLastSeen.end() &&
            std::chrono::duration<float>(ahora - seen->second).count() > 5.0f) {
            m_remoteLastSeen.erase(seen);
            it = m_remotePlayers.erase(it);
        } else {
            ++it;
        }
    }
    return m_remotePlayers;
}

}
