#include "NetworkClient.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstring>

namespace Network {

NetworkClient& NetworkClient::GetInstance() {
    static NetworkClient instance;
    return instance;
}

bool NetworkClient::Initialize() {
    if (m_initialized) return true;

    spdlog::info("[Network] Initializing GameNetworkingSockets...");

    SteamNetworkingErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
        spdlog::error("[Network] Failed to init GameNetworkingSockets: {}", errMsg);
        return false;
    }

    m_sockets = SteamNetworkingSockets();
    m_utils = SteamNetworkingUtils();

    if (!m_sockets || !m_utils) {
        spdlog::error("[Network] Failed to get networking interfaces");
        return false;
    }

    // Permitir conexiones IP sin autenticacion de Steam.
    m_utils->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);

    // Registrar el callback global de cambios de estado de conexion.
    m_utils->SetGlobalCallback_SteamNetConnectionStatusChanged(&NetworkClient::OnConnStatusChangedStatic);

    m_initialized = true;
    spdlog::info("[Network] GameNetworkingSockets initialized");
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

    spdlog::info("[Network] Network client shutdown");
}

bool NetworkClient::Connect(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_connection != k_HSteamNetConnection_Invalid) {
        spdlog::warn("[Network] Already connecting/connected");
        return false;
    }

    char addrStr[64];
    snprintf(addrStr, sizeof(addrStr), "%s:%d", address.c_str(), port);

    SteamNetworkingIPAddr serverAddr;
    serverAddr.Clear();
    if (!serverAddr.ParseString(addrStr)) {
        spdlog::error("[Network] Invalid server address: {}", addrStr);
        return false;
    }

    m_connection = m_sockets->ConnectByIPAddress(serverAddr, 0, nullptr);
    if (m_connection == k_HSteamNetConnection_Invalid) {
        spdlog::error("[Network] Failed to create connection to {}", addrStr);
        return false;
    }

    spdlog::info("[Network] Connecting to {}...", addrStr);
    return true;
}

void NetworkClient::Disconnect() {
    if (m_connection != k_HSteamNetConnection_Invalid) {
        m_sockets->CloseConnection(m_connection, 0, "Disconnecting", false);
        m_connection = k_HSteamNetConnection_Invalid;
    }

    m_connected = false;
    m_playerId = 0;

    if (m_connectionCallback) {
        m_connectionCallback(false);
    }

    spdlog::info("[Network] Disconnected from server");
}

void NetworkClient::SendPacket(MessageType type, const void* data, uint32_t size) {
    if (m_connection == k_HSteamNetConnection_Invalid) {
        spdlog::warn("[Network] Cannot send: no connection");
        return;
    }

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

    EResult result = m_sockets->SendMessageToConnection(
        m_connection, buffer.data(), (uint32_t)buffer.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);

    if (result != k_EResultOK) {
        spdlog::error("[Network] Failed to send message: {}", (int)result);
    }
}

void NetworkClient::SendConnectionRequest() {
    ConnectionRequestMsg req{};
    strncpy(req.playerName, m_playerName.c_str(), sizeof(req.playerName) - 1);
    req.protocolVersion = PROTOCOL_VERSION;
    SendPacket(MessageType::ConnectionRequest, &req, sizeof(req));
    spdlog::info("[Network] Sent connection request as '{}'", m_playerName);
}

void NetworkClient::SetMessageCallback(std::function<void(const MessageHeader&, const void*)> callback) {
    m_messageCallback = std::move(callback);
}

void NetworkClient::SetConnectionCallback(std::function<void(bool)> callback) {
    m_connectionCallback = std::move(callback);
}

void NetworkClient::OnConnStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* info) {
    GetInstance().OnConnStatusChanged(info);
}

void NetworkClient::OnConnStatusChanged(SteamNetConnectionStatusChangedCallback_t* info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connected:
            // El socket esta arriba: enviamos el handshake de aplicacion.
            spdlog::info("[Network] Transport connected, sending handshake...");
            SendConnectionRequest();
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            spdlog::warn("[Network] Connection closed: {}", info->m_info.m_szEndDebug);
            if (m_connection != k_HSteamNetConnection_Invalid) {
                m_sockets->CloseConnection(m_connection, 0, nullptr, false);
                m_connection = k_HSteamNetConnection_Invalid;
            }
            m_connected = false;
            m_playerId = 0;
            if (m_connectionCallback) m_connectionCallback(false);
            break;

        default:
            break;
    }
}

void NetworkClient::PumpMessages() {
    if (!m_initialized) return;

    // 1. Deja que GNS procese estados de conexion (dispara OnConnStatusChanged).
    m_sockets->RunCallbacks();

    if (m_connection == k_HSteamNetConnection_Invalid) return;

    // 2. Procesa mensajes entrantes.
    SteamNetworkingMessage_t* msgs[16];
    int msgCount = m_sockets->ReceiveMessagesOnConnection(m_connection, msgs, 16);

    for (int i = 0; i < msgCount; ++i) {
        SteamNetworkingMessage_t* msg = msgs[i];
        if (msg->m_cbSize >= (int)sizeof(MessageHeader)) {
            const auto* header = reinterpret_cast<const MessageHeader*>(msg->m_pData);
            const void* payload = reinterpret_cast<const uint8_t*>(msg->m_pData) + sizeof(MessageHeader);

            if (header->type == MessageType::ConnectionAccepted) {
                const auto* acceptMsg = reinterpret_cast<const ConnectionAcceptedMsg*>(payload);
                m_playerId = acceptMsg->playerId;
                m_connected = true;
                spdlog::info("[Network] Connected! Player ID: {}, Players: {}/{}",
                    m_playerId, acceptMsg->currentPlayers, acceptMsg->maxPlayers);
                if (m_connectionCallback) m_connectionCallback(true);
            } else if (header->type == MessageType::ConnectionRejected) {
                spdlog::error("[Network] Connection rejected by server");
                if (m_connectionCallback) m_connectionCallback(false);
            } else if (m_messageCallback) {
                m_messageCallback(*header, payload);
            }
        }
        msg->Release();
    }
}

}
