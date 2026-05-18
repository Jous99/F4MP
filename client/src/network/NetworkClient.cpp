#include "NetworkClient.h"
#include "f4mp/Logger.h"

#include <chrono>

namespace f4mp {

NetworkClient& NetworkClient::GetInstance() {
    static NetworkClient instance;
    return instance;
}

bool NetworkClient::Initialize() {
    if (m_initialized) {
        return true;
    }

    Logger::Info("Initializing network client");

    SteamNetworkingErrMsg errMsg;
    if (!SteamNetworkingSockets_Init(nullptr, errMsg)) {
        Logger::Error("Failed to initialize GameNetworkingSockets: %s", errMsg);
        return false;
    }

    m_sockets = SteamNetworkingSockets();
    m_utils = SteamNetworkingUtils();

    if (!m_sockets || !m_utils) {
        Logger::Error("Failed to get networking interfaces");
        return false;
    }

    Logger::Info("GameNetworkingSockets initialized");
    m_initialized = true;
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

    Logger::Info("Network client shutdown");
}

bool NetworkClient::Connect(const std::string& address, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_connected) {
        Logger::Warn("Already connected");
        return false;
    }

    char addrStr[64];
    snprintf(addrStr, sizeof(addrStr), "%s:%d", address.c_str(), port);

    SteamNetworkingIPAddr serverAddr;
    serverAddr.Clear();
    if (!serverAddr.ParseString(addrStr)) {
        Logger::Error("Invalid server address: %s", addrStr);
        return false;
    }

    int opt = 1;
    m_sockets->SetConnectionConfigValueInt(k_HSteamNetConnection_Invalid, k_ESteamNetworkingConfig_IP_AllowWithoutAuth, opt);

    m_connection = m_sockets->ConnectByIPAddress(serverAddr, 0, nullptr);

    if (m_connection == k_HSteamNetConnection_Invalid) {
        Logger::Error("Failed to create connection to %s", addrStr);
        return false;
    }

    Logger::Info("Connecting to %s...", addrStr);
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

    Logger::Info("Disconnected from server");
}

void NetworkClient::SendMessage(MessageType type, const void* data, uint32_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_connected || m_connection == k_HSteamNetConnection_Invalid) {
        Logger::Warn("Cannot send message: not connected");
        return;
    }

    MessageHeader header;
    header.type = type;
    header.size = sizeof(MessageHeader) + size;
    header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    header.sequence = m_sequence++;

    std::vector<uint8_t> buffer(header.size);
    memcpy(buffer.data(), &header, sizeof(MessageHeader));
    if (data && size > 0) {
        memcpy(buffer.data() + sizeof(MessageHeader), data, size);
    }

    EResult result = m_sockets->SendMessageToConnection(
        m_connection,
        buffer.data(),
        buffer.size(),
        k_nSteamNetworkingSend_Reliable,
        nullptr
    );

    if (result != k_EResultOK) {
        Logger::Error("Failed to send message: %d", result);
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

            if (m_messageCallback) {
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

            if (m_messageCallback) {
                m_messageCallback(*header, payload);
            }
        }
        msg->Release();
    }
}

void NetworkClient::OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo) {
    switch (pInfo->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connecting:
            Logger::Info("Connecting to server...");
            break;

        case k_ESteamNetworkingConnectionState_Connected:
            Logger::Info("Connected to server");
            m_connected = true;
            if (m_connectionCallback) {
                m_connectionCallback(true);
            }
            break;

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
            Logger::Warn("Connection closed: %d", pInfo->m_info.m_eState);
            m_connected = false;
            m_connection = k_HSteamNetConnection_Invalid;
            if (m_connectionCallback) {
                m_connectionCallback(false);
            }
            break;

        default:
            break;
    }
}

}
