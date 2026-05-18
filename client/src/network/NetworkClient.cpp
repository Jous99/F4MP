#include "NetworkClient.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace Network {

NetworkClient& NetworkClient::GetInstance() {
    static NetworkClient instance;
    return instance;
}

bool NetworkClient::Initialize() {
    if (m_initialized) return true;

    spdlog::info("[Network] Initializing GameNetworkingSockets...");

    SteamNetworkingErrMsg errMsg;
    if (!SteamNetworkingSockets_Init(nullptr, errMsg)) {
        spdlog::error("[Network] Failed to init GameNetworkingSockets: {}", errMsg);
        return false;
    }

    m_sockets = SteamNetworkingSockets();
    m_utils = SteamNetworkingUtils();

    if (!m_sockets || !m_utils) {
        spdlog::error("[Network] Failed to get networking interfaces");
        return false;
    }

    int opt = 1;
    m_sockets->SetConnectionConfigValueInt(k_HSteamNetConnection_Invalid,
        k_ESteamNetworkingConfig_IP_AllowWithoutAuth, opt);

    m_initialized = true;
    spdlog::info("[Network] GameNetworkingSockets initialized");
    return true;
}

void NetworkClient::Shutdown() {
    if (m_connected) {
        Disconnect();
    }

    if (m_initialized) {
        SteamNetworkingSockets_Kill();
        m_initialized = false;
    }

    spdlog::info("[Network] Network client shutdown");
}

bool NetworkClient::Connect(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_connected) {
        spdlog::warn("[Network] Already connected");
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
    std::lock_guard<std::mutex> lock(m_mutex);

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

void NetworkClient::SendMessage(MessageType type, const void* data, uint32_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_connected || m_connection == k_HSteamNetConnection_Invalid) {
        spdlog::warn("[Network] Cannot send: not connected");
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
        m_connection, buffer.data(), buffer.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);

    if (result != k_EResultOK) {
        spdlog::error("[Network] Failed to send message: {}", result);
    }
}

void NetworkClient::SetMessageCallback(std::function<void(const MessageHeader&, const void*)> callback) {
    m_messageCallback = std::move(callback);
}

void NetworkClient::SetConnectionCallback(std::function<void(bool)> callback) {
    m_connectionCallback = std::move(callback);
}

void NetworkClient::PumpMessages() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || m_connection == k_HSteamNetConnection_Invalid) return;

    SteamNetworkingMessage_t* msg = nullptr;
    int msgCount = m_sockets->ReceiveMessagesOnConnection(m_connection, &msg, 1, 0);

    while (msgCount > 0 && msg) {
        if (msg->m_cbSize >= sizeof(MessageHeader)) {
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
        msgCount = m_sockets->ReceiveMessagesOnConnection(m_connection, &msg, 1, 0);
    }

    SteamNetConnectionStatusChangedCallback_t statusChange;
    while (m_sockets->ReceiveMessagesOnConnection(m_connection, &msg, 1, 0) > 0 && msg) {
        if (msg->m_cbSize >= sizeof(MessageHeader)) {
            const auto* header = reinterpret_cast<const MessageHeader*>(msg->m_pData);
            const void* payload = reinterpret_cast<const uint8_t*>(msg->m_pData) + sizeof(MessageHeader);
            if (m_messageCallback) m_messageCallback(*header, payload);
        }
        msg->Release();
    }
}

}
