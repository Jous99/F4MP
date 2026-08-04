#include "GameServer.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <cstring>

GameServer& GameServer::GetInstance() {
    static GameServer instance;
    return instance;
}

bool GameServer::Initialize(uint16_t port) {
    if (m_initialized) return true;

    spdlog::info("[Server] Initializing on port {}", port);

    SteamNetworkingErrMsg errMsg;
    if (!GameNetworkingSockets_Init(nullptr, errMsg)) {
        spdlog::error("[Server] Failed to init GameNetworkingSockets: {}", errMsg);
        return false;
    }

    m_sockets = SteamNetworkingSockets();
    if (!m_sockets) {
        spdlog::error("[Server] Failed to get networking interface");
        return false;
    }

    // Permitir conexiones IP sin autenticacion de Steam.
    SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, 1);

    // Registrar el callback global de cambios de estado de conexion.
    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(&GameServer::OnConnStatusChangedStatic);

    SteamNetworkingIPAddr bindAddr;
    bindAddr.Clear();
    bindAddr.m_port = port;

    m_listenSocket = m_sockets->CreateListenSocketIP(bindAddr, 0, nullptr);
    if (m_listenSocket == k_HSteamListenSocket_Invalid) {
        spdlog::error("[Server] Failed to create listen socket on port {}", port);
        GameNetworkingSockets_Kill();
        return false;
    }

    m_pollGroup = m_sockets->CreatePollGroup();
    if (m_pollGroup == k_HSteamNetPollGroup_Invalid) {
        spdlog::error("[Server] Failed to create poll group");
        GameNetworkingSockets_Kill();
        return false;
    }

    spdlog::info("[Server] Listening on port {} (max players: {})", port, m_maxPlayers);
    m_initialized = true;
    m_running = true;
    return true;
}

void GameServer::Shutdown() {
    m_running = false;

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        for (auto& [conn, client] : m_clients) {
            m_sockets->CloseConnection(conn, 0, "Server shutting down", false);
        }
        m_clients.clear();
    }

    if (m_pollGroup != k_HSteamNetPollGroup_Invalid) {
        m_sockets->DestroyPollGroup(m_pollGroup);
        m_pollGroup = k_HSteamNetPollGroup_Invalid;
    }

    if (m_listenSocket != k_HSteamListenSocket_Invalid) {
        m_sockets->CloseListenSocket(m_listenSocket);
        m_listenSocket = k_HSteamListenSocket_Invalid;
    }

    if (m_initialized) {
        GameNetworkingSockets_Kill();
        m_initialized = false;
    }

    spdlog::info("[Server] Shutdown complete");
}

void GameServer::Run() {
    spdlog::info("[Server] Main loop started");

    auto lastTick = std::chrono::steady_clock::now();

    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - lastTick).count();

        if (elapsed >= 1.0f / 30.0f) {
            Update();
            lastTick = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServer::Update() {
    // Procesa estados de conexion (dispara OnConnStatusChanged) y mensajes.
    m_sockets->RunCallbacks();
    ProcessMessages();
}

uint32_t GameServer::GetPlayerCount() const {
    uint32_t count = 0;
    for (const auto& [conn, client] : m_clients) {
        if (client.isConnected) count++;
    }
    return count;
}

void GameServer::HandleConnectionRequest(HSteamNetConnection conn, const ConnectionRequestMsg* msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    if (m_clients.size() >= m_maxPlayers) {
        spdlog::warn("[Server] Connection rejected: server full");
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
    acceptMsg.currentPlayers = GetPlayerCount();
    acceptMsg.maxPlayers = m_maxPlayers;

    // Enviamos el ConnectionAccepted con su cabecera para que el cliente lo entienda.
    MessageHeader header;
    header.type = MessageType::ConnectionAccepted;
    header.size = sizeof(MessageHeader) + sizeof(acceptMsg);
    header.timestamp = 0;
    std::vector<uint8_t> buffer(header.size);
    memcpy(buffer.data(), &header, sizeof(MessageHeader));
    memcpy(buffer.data() + sizeof(MessageHeader), &acceptMsg, sizeof(acceptMsg));
    m_sockets->SendMessageToConnection(conn, buffer.data(), (uint32_t)buffer.size(),
        k_nSteamNetworkingSend_Reliable, nullptr);

    spdlog::info("[Server] Player '{}' connected (ID: {})", client.name.c_str(), client.id);

    ChatMessageMsg welcomeMsg{};
    welcomeMsg.senderId = 0;
    snprintf(welcomeMsg.message, sizeof(welcomeMsg.message), "%s has joined the wasteland", client.name.c_str());
    welcomeMsg.channel = 0;

    BroadcastMessage(MessageType::ChatMessage, &welcomeMsg, sizeof(welcomeMsg), client.id);
}

void GameServer::HandlePlayerPosition(HSteamNetConnection conn, const PlayerPositionMsg* msg) {
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

void GameServer::HandleChatMessage(HSteamNetConnection conn, const ChatMessageMsg* msg) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    auto it = m_clients.find(conn);
    if (it != m_clients.end()) {
        ChatMessageMsg broadcastMsg = *msg;
        broadcastMsg.senderId = it->second.id;
        BroadcastMessage(MessageType::ChatMessage, &broadcastMsg, sizeof(broadcastMsg), 0);
    }
}

void GameServer::BroadcastMessage(MessageType type, const void* data, uint32_t size, uint32_t excludeId) {
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

    for (const auto& [conn, client] : m_clients) {
        if (client.isConnected && client.id != excludeId) {
            m_sockets->SendMessageToConnection(conn, buffer.data(), (uint32_t)buffer.size(),
                k_nSteamNetworkingSend_Reliable, nullptr);
        }
    }
}

void GameServer::SendMessageToClient(uint32_t clientId, MessageType type, const void* data, uint32_t size) {
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    for (const auto& [conn, client] : m_clients) {
        if (client.id == clientId) {
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

            m_sockets->SendMessageToConnection(conn, buffer.data(), (uint32_t)buffer.size(),
                k_nSteamNetworkingSend_Reliable, nullptr);
            break;
        }
    }
}

void GameServer::ProcessMessages() {
    SteamNetworkingMessage_t* msgs[32];
    int msgCount = m_sockets->ReceiveMessagesOnPollGroup(m_pollGroup, msgs, 32);

    for (int i = 0; i < msgCount; ++i) {
        SteamNetworkingMessage_t* msg = msgs[i];
        if (msg->m_cbSize >= (int)sizeof(MessageHeader)) {
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
                    spdlog::debug("[Server] Unhandled message type: {}", static_cast<int>(header->type));
                    break;
            }
        }
        msg->Release();
    }
}

void GameServer::OnConnStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* info) {
    GetInstance().OnConnStatusChanged(info);
}

void GameServer::OnConnStatusChanged(SteamNetConnectionStatusChangedCallback_t* info) {
    switch (info->m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connecting: {
            // Nueva conexion entrante: aceptar y meterla en el poll group.
            if (m_sockets->AcceptConnection(info->m_hConn) != k_EResultOK) {
                m_sockets->CloseConnection(info->m_hConn, 0, nullptr, false);
                spdlog::warn("[Server] Failed to accept connection");
                break;
            }
            m_sockets->SetConnectionPollGroup(info->m_hConn, m_pollGroup);
            spdlog::info("[Server] Connection accepted, waiting for handshake");
            break;
        }

        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            auto it = m_clients.find(info->m_hConn);
            if (it != m_clients.end()) {
                spdlog::info("[Server] Player '{}' disconnected", it->second.name.c_str());

                ChatMessageMsg leaveMsg{};
                leaveMsg.senderId = 0;
                snprintf(leaveMsg.message, sizeof(leaveMsg.message), "%s has left the wasteland", it->second.name.c_str());
                leaveMsg.channel = 0;

                uint32_t playerId = it->second.id;
                m_clients.erase(it);

                BroadcastMessage(MessageType::ChatMessage, &leaveMsg, sizeof(leaveMsg), playerId);
            }
            m_sockets->CloseConnection(info->m_hConn, 0, nullptr, false);
            break;
        }

        default:
            break;
    }
}
