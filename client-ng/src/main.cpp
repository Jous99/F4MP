#include "pch.h"
#include "F4SEMenuFramework.h"
#include "network/NetworkClient.h"

#include <thread>
#include <chrono>
#include <atomic>

// HITO 2 + 3: menu in-game (F4SE Menu Framework) + red real.
// El menu conecta/desconecta y un hilo de red envia tu posicion al servidor.

namespace F4MP
{
    static char g_serverAddr[64] = "127.0.0.1";
    static int  g_serverPort = 7779;
    static char g_playerName[64] = "Wastelander";
    static std::atomic<bool> g_netThreadStarted{ false };

    // ---- Hilo de red: procesa mensajes y envia la posicion ----
    static void NetThread()
    {
        using namespace std::chrono;
        auto lastSend = steady_clock::now();

        for (;;) {
            auto& net = Network::NetworkClient::GetInstance();
            net.PumpMessages();

            if (net.IsConnected()) {
                auto now = steady_clock::now();
                if (duration_cast<milliseconds>(now - lastSend).count() >= 100) {  // 10/s
                    lastSend = now;
                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        const auto pos = player->GetPosition();
                        Network::PlayerPositionMsg msg{};
                        msg.playerId = net.GetPlayerId();
                        msg.x = pos.x;
                        msg.y = pos.y;
                        msg.z = pos.z;
                        msg.cellId = 0;
                        net.SendPacket(Network::MessageType::PlayerPosition, &msg, sizeof(msg));
                    }
                }
            }

            std::this_thread::sleep_for(milliseconds(30));
        }
    }

    // ---- Pagina del menu (la dibuja el framework) ----
    static void __stdcall RenderMenu()
    {
        auto& net = Network::NetworkClient::GetInstance();

        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            const auto pos = player->GetPosition();
            ImGuiMCP::Text("Tu posicion:  x=%.0f  y=%.0f  z=%.0f", pos.x, pos.y, pos.z);
        }

        ImGuiMCP::Separator();

        ImGuiMCP::InputText("Nombre", g_playerName, sizeof(g_playerName));
        ImGuiMCP::InputText("Servidor", g_serverAddr, sizeof(g_serverAddr));
        ImGuiMCP::InputInt("Puerto", &g_serverPort);

        if (net.IsConnected()) {
            ImGuiMCP::Text("Estado: CONECTADO (ID %u)", net.GetPlayerId());
            if (ImGuiMCP::Button("Desconectar")) {
                net.Disconnect();
            }
        } else {
            ImGuiMCP::Text("Estado: desconectado");
            if (ImGuiMCP::Button("Conectar")) {
                net.SetPlayerName(g_playerName);
                net.Connect(g_serverAddr, static_cast<uint16_t>(g_serverPort));
            }
        }
    }

    static void RegisterMenu()
    {
        if (!F4SEMenuFramework::IsInstalled()) {
            REX::INFO("[F4MP] F4SE Menu Framework NO instalado: menu no disponible");
            return;
        }
        F4SEMenuFramework::SetSection("F4MP");
        F4SEMenuFramework::AddSectionItem("Menu", RenderMenu);
        REX::INFO("[F4MP] menu registrado");
    }

    static void OnMessage(F4SE::MessagingInterface::Message* a_msg)
    {
        if (!a_msg) return;

        if (a_msg->type == F4SE::MessagingInterface::kPostLoad ||
            a_msg->type == F4SE::MessagingInterface::kPostPostLoad) {
            RegisterMenu();
        }

        if (a_msg->type == F4SE::MessagingInterface::kGameDataReady) {
            if (!g_netThreadStarted.exchange(true)) {
                Network::NetworkClient::GetInstance().Initialize();
                std::thread(NetThread).detach();
                REX::INFO("[F4MP] red inicializada, hilo de red en marcha");
            }
        }
    }
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
    F4SE::Init(a_f4se);
    REX::INFO("[F4MP] Plugin cargado (CommonLibF4)");

    F4SE::GetMessagingInterface()->RegisterListener(F4MP::OnMessage);
    return true;
}
