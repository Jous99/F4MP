#include "Server.h"
#include "f4mp/Logger.h"

#include <chrono>
#include <thread>

namespace f4mp {

Server& Server::GetInstance() {
    static Server instance;
    return instance;
}

bool Server::Initialize(uint16_t port) {
    if (m_initialized) {
        return true;
    }

    Logger::Info("Initializing F4MP Server on port %d", port);

    SteamNetworkingErrMsg errMsg;
    if (!SteamNetworkingSockets_Init(nullptr, errMsg)) {
        Logger::Error("Failed to initialize GameNetworkingSockets: %s", errMsg);
        return false;
    }

    m_sockets = SteamNetworkingSockets();
    if (!m_sockets) {
        Logger::Error("Failed to get networking sockets interface");
        return false;
    }

    SteamNetworkingIPAddr bindAddr;
    bindAddr.Clear();
    bindAddr.m_port = port;

    int opt = 1;
    m_sockets->SetConnectionConfigValueInt(k_HSteamNetConnection_Invalid, k_ESteamNetworkingConfig_IP_AllowWithoutAuth, opt);

    m_listenSocket = m_sockets->CreateListenSocketIP(bindAddr, 0, nullptr);
    if (m_listenSocket == k_HSteamListenSocket_Invalid) {
        Logger::Error("Failed to create listen socket on port %d", port);
        SteamNetworkingSockets_Kill();
        return false;
    }

    Logger::Info("Server listening on port %d", port);
    m_initialized = true;
    m_running = true;
    return true;
}

void Server::Shutdown() {
    m_running = false;

    if (m_listenSocket != k_HSteamListenSocket_Invalid) {
        m_sockets->CloseListenSocket(m_listenSocket);
        m_listenSocket = k_HSteamListenSocket_Invalid;
    }

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& [conn, client] : m_clients) {
            m_sockets->CloseConnection(conn, 0, "Server shutting down", false);
        }
        m_clients.clear();
    }

    if (m_initialized) {
        SteamNetworkingSockets_Kill();
        m_initialized = false;
    }

    Logger::Info("Server shutdown");
}

void Server::Run() {
    Logger::Info("Server main loop started");

    auto lastTick = std::chrono::steady_clock::now();

    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - lastTick).count();

        if (elapsed >= TICK_INTERVAL) {
            Update();
            lastTick = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Server::Update() {
    ProcessMessages();
    HandleConnectionStatusChanges();
}

uint32_t Server::GetPlayerCount() const {
    uint32_t count = 0;
    for (const auto& [conn, client] : m_clients) {
        if (client.isConnected) {
            count++;
        }
    }
    return count;
}

void Server::HandleConnectionRequest(HSteamNetConnection conn, const ConnectionRequestMsg* msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    if (m_clients.size() >= m_maxPlayers) {
        Logger::Warn("Connection rejected: server full");

        ConnectionAcceptedMsg rejectMsg{};
        rejectMsg.playerId = 0;
        rejectMsg.currentPlayers = m_maxPlayers;
        rejectMsg.maxPlayers = m_maxPlayers;

        m_sockets->SendMessageToConnection(conn, &rejectMsg, sizeof(rejectMsg), k_nSteamNetworkingSend_Reliable, nullptr);
        m_sockets->CloseConnection(conn, 0, "Server full", false);
        return;
    }

    ClientInfo client{};
    client.id = m_nextClientId++;
    client.connection = conn;
    client.name = msg->playerName;
    client.isConnected = true;
    client.lastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0;

    m_clients[conn] = client;

    ConnectionAcceptedMsg acceptMsg{};
    acceptMsg.playerId = client.id;
    acceptMsg.sessionId = msg->sessionId;
    acceptMsg.currentPlayers = GetPlayerCount();
    acceptMsg.maxPlayers = m_maxPlayers;

    m_sockets->SendMessageToConnection(conn, &acceptMsg, sizeof(acceptMsg), k_nSteamNetworkingSend_Reliable, nullptr);

    Logger::Info("Player '%s' connected with ID %d", client.name.c_str(), client.id);

    ChatMessageMsg welcomeMsg{};
    welcomeMsg.senderId = 0;
    snprintf(welcomeMsg.message, sizeof(welcomeMsg.message), "%s has joined the wasteland", client.name.c_str());
    welcomeMsg.channel = 0;

    BroadcastMessage(MessageType::ChatBroadcast, &welcomeMsg, sizeof(welcomeMsg), client.id);
}

void Server::HandlePlayerPosition(HSteamNetConnection conn, const PlayerPositionMsg* msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    auto it = m_clients.find(conn);
    if (it != m_clients.end()) {
        it->second.x = msg->x;
        it->second.y = msg->y;
        it->second.z = msg->z;
        it->second.cellId = msg->cellId;
        it->second.lastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() / 1000.0;

        BroadcastMessage(MessageType::PlayerPosition, msg, sizeof(PlayerPositionMsg), it->second.id);
    }
}

void Server::HandleChatMessage(HSteamNetConnection conn, const ChatMessageMsg* msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    auto it = m_clients.find(conn);
    if (it != m_clients.end()) {
        ChatMessageMsg broadcastMsg = *msg;
        broadcastMsg.senderId = it->second.id;

        BroadcastMessage(MessageType::ChatBroadcast, &broadcastMsg, sizeof(broadcastMsg), 0);
    }
}

void Server::BroadcastMessage(MessageType type, const void* data, uint32_t size, uint32_t excludeId) {
    MessageHeader header;
    header.type = type;
    header.size = sizeof(MessageHeader) + size;
    header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    header.sequence = 0;

    std::vector<uint8_t> buffer(header.size);
    memcpy(buffer.data(), &header, sizeof(MessageHeader));
    if (data && size > 0) {
        memcpy(buffer.data() + sizeof(MessageHeader), data, size);
    }

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    for (const auto& [conn, client] : m_clients) {
        if (client.isConnected && client.id != excludeId) {
            m_sockets->SendMessageToConnection(conn, buffer.data(), buffer.size(), k_nSteamNetworkingSend_Reliable, nullptr);
        }
    }
}

void Server::SendMessageToClient(uint32_t clientId, MessageType type, const void* data, uint32_t size) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    for (const auto& [conn, client] : m_clients) {
        if (client.id == clientId) {
            MessageHeader header;
            header.type = type;
            header.size = sizeof(MessageHeader) + size;
            header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            header.sequence = 0;

            std::vector<uint8_t> buffer(header.size);
            memcpy(buffer.data(), &header, sizeof(MessageHeader));
            if (data && size > 0) {
                memcpy(buffer.data() + sizeof(MessageHeader), data, size);
            }

            m_sockets->SendMessageToConnection(conn, buffer.data(), buffer.size(), k_nSteamNetworkingSend_Reliable, nullptr);
            break;
        }
    }
}

void Server::ProcessMessages() {
    SteamNetworkingMessage_t* msg = nullptr;
    int msgCount = m_sockets->ReceiveMessagesOnListenSocket(m_listenSocket, &msg, 1, 0);

    while (msgCount > 0 && msg) {
        if (msg->m_cbSize >= sizeof(MessageHeader)) {
            const auto* header = reinterpret_cast<const MessageHeader*>(msg->m_pData);
            const void* payload = reinterpret_cast<const uint8_t*>(msg->m_pData) + sizeof(MessageHeader);

            switch (header->type) {
                case MessageType::ConnectionRequest:
                    HandleConnectionRequest(msg->m_conn, reinterpret_cast<const ConnectionRequestMsg*>(payload));
                    break;
                case MessageType::PlayerPosition:
                    HandlePlayerPosition(msg->m_conn, reinterpret_cast<const PlayerPositionMsg*>(payload));
                    break;
                case MessageType::ChatMessage:
                    HandleChatMessage(msg->m_conn, reinterpret_cast<const ChatMessageMsg*>(payload));
                    break;
                default:
                    Logger::Debug("Unhandled message type: %d", static_cast<int>(header->type));
                    break;
            }
        }
        msg->Release();

        msgCount = m_sockets->ReceiveMessagesOnListenSocket(m_listenSocket, &msg, 1, 0);
    }
}

void Server::HandleConnectionStatusChanges() {
    SteamNetConnectionStatusChangedCallback_t statusChange;
    while (m_sockets->ReceiveMessagesOnListenSocket(m_listenSocket, &statusChange, 1, 0) > 0) {
        switch (statusChange.m_info.m_eState) {
            case k_ESteamNetworkingConnectionState_Connecting:
                Logger::Info("New connection attempt");
                m_sockets->AcceptConnection(statusChange.m_hConn);
                break;

            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                std::lock_guard<std::mutex> lock(m_clientsMutex);
                auto it = m_clients.find(statusChange.m_hConn);
                if (it != m_clients.end()) {
                    Logger::Info("Player '%s' disconnected", it->second.name.c_str());

                    ChatMessageMsg leaveMsg{};
                    leaveMsg.senderId = 0;
                    snprintf(leaveMsg.message, sizeof(leaveMsg.message), "%s has left the wasteland", it->second.name.c_str());
                    leaveMsg.channel = 0;

                    uint32_t playerId = it->second.id;
                    m_clients.erase(it);

                    BroadcastMessage(MessageType::ChatBroadcast, &leaveMsg, sizeof(leaveMsg), playerId);
                }
                break;
            }

            default:
                break;
        }
    }
}

}
