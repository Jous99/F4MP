#pragma once

#include "f4mp/NetworkProtocol.h"

#include <steamnetworkingsockets/steamnetworkingsockets.h>
#include <functional>
#include <string>
#include <vector>
#include <mutex>

namespace f4mp {

class NetworkClient {
public:
    static NetworkClient& GetInstance();

    bool Initialize();
    void Shutdown();

    bool Connect(const std::string& address, uint16_t port);
    void Disconnect();

    bool IsConnected() const { return m_connected; }

    void SendMessage(MessageType type, const void* data, uint32_t size);

    void SetMessageCallback(std::function<void(const MessageHeader&, const void*)> callback);
    void SetConnectionCallback(std::function<void(bool)> callback);

    void PumpMessages();

    uint32_t GetPlayerId() const { return m_playerId; }

private:
    NetworkClient() = default;

    void OnConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* pInfo);

    ISteamNetworkingSockets* m_sockets = nullptr;
    ISteamNetworkingUtils* m_utils = nullptr;

    HSteamListenSocket m_listenSocket = k_HSteamListenSocket_Invalid;
    HSteamNetConnection m_connection = k_HSteamNetConnection_Invalid;

    bool m_connected = false;
    bool m_initialized = false;

    uint32_t m_playerId = 0;
    uint32_t m_sequence = 0;

    std::mutex m_mutex;

    std::function<void(const MessageHeader&, const void*)> m_messageCallback;
    std::function<void(bool)> m_connectionCallback;
};

}
