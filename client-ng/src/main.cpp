#include "pch.h"
#include "F4SEMenuFramework.h"
#include "network/NetworkClient.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <cmath>
#include <cstdio>

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

    // ---- PRUEBA de spawn: coloca un Protectron en tu posicion via Papyrus ----
    // Llama a ObjectReference.PlaceAtMe por la VM (independiente de version, sin
    // direcciones fijas). Es solo para comprobar que spawnear funciona.
    static void SpawnDummy()
    {
        // Ejecuta el comando de consola (que sabemos que funciona) por su ID de
        // Address Library. Evita el fragil empaquetado de argumentos de Papyrus.
        REX::INFO("[F4MP] spawn: ejecutando 'player.placeatme 00106B09 1'");
        RE::Console::ExecuteCommand("player.placeatme 00106B09 1");
        REX::INFO("[F4MP] spawn: comando enviado");
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

            ImGuiMCP::Separator();
            auto remotos = net.GetRemotePlayers();
            ImGuiMCP::Text("Jugadores remotos: %d", (int)remotos.size());
            for (const auto& [id, p] : remotos) {
                ImGuiMCP::Text("  #%u  x=%.0f y=%.0f z=%.0f", id, p.x, p.y, p.z);
            }
        } else {
            ImGuiMCP::Text("Estado: desconectado");
            if (ImGuiMCP::Button("Conectar")) {
                net.SetPlayerName(g_playerName);
                net.Connect(g_serverAddr, static_cast<uint16_t>(g_serverPort));
            }
        }

        ImGuiMCP::Separator();
        if (ImGuiMCP::Button("Spawn dummy (test)")) {
            // Ejecutar en el hilo principal del juego (mas seguro para Papyrus).
            F4SE::GetTaskInterface()->AddTask([]() { SpawnDummy(); });
        }
    }

    // ---- HUD: marcador flotante sobre cada jugador remoto (mundo -> pantalla) ----
    static void __stdcall RenderHud()
    {
        auto& net = Network::NetworkClient::GetInstance();
        if (!net.IsConnected()) return;

        auto* local = RE::PlayerCharacter::GetSingleton();
        if (!local) return;
        const RE::NiPoint3 myPos = local->GetPosition();

        auto* io = ImGuiMCP::GetIO();
        if (!io) return;
        const float W = io->DisplaySize.x;
        const float H = io->DisplaySize.y;

        ImGuiMCP::ImDrawList* dl = ImGuiMCP::GetForegroundDrawList();
        if (!dl) return;

        const ImGuiMCP::ImU32 col = 0xFF00FF00u;  // verde (ABGR)

        auto remotos = net.GetRemotePlayers();
        for (const auto& [id, p] : remotos) {
            // Proyectamos pies y cabeza para un marcador con altura de persona.
            RE::NiPoint3 feetW{ p.x, p.y, p.z };
            RE::NiPoint3 headW{ p.x, p.y, p.z + 120.0f };  // ~altura (unidades FO4)

            RE::NiPoint3 feetS = RE::HUDMenuUtils::WorldPtToScreenPt3(feetW);
            RE::NiPoint3 headS = RE::HUDMenuUtils::WorldPtToScreenPt3(headW);

            if (feetS.z <= 0.0f) continue;  // detras de la camara
            if (feetS.x < -0.2f || feetS.x > 1.2f || feetS.y < -0.2f || feetS.y > 1.2f) continue;

            const float fx = feetS.x * W, fy = (1.0f - feetS.y) * H;
            const float hx = headS.x * W, hy = (1.0f - headS.y) * H;

            const float dx = p.x - myPos.x, dy = p.y - myPos.y, dz = p.z - myPos.z;
            const float dist = std::sqrt(dx * dx + dy * dy + dz * dz) / 100.0f;

            // "cuerpo": linea de pies a cabeza
            ImGuiMCP::ImDrawListManager::AddLine(dl, ImGuiMCP::ImVec2(fx, fy), ImGuiMCP::ImVec2(hx, hy), col, 3.0f);
            // cabeza
            ImGuiMCP::ImDrawListManager::AddCircleFilled(dl, ImGuiMCP::ImVec2(hx, hy), 6.0f, col, 16);
            // pies
            ImGuiMCP::ImDrawListManager::AddCircle(dl, ImGuiMCP::ImVec2(fx, fy), 4.0f, col, 12, 2.0f);

            char label[64];
            snprintf(label, sizeof(label), "Jugador %u  (%.0f m)", id, dist);
            ImGuiMCP::ImDrawListManager::AddText(dl, ImGuiMCP::ImVec2(hx + 8.0f, hy - 16.0f), col, label);
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
        F4SEMenuFramework::AddHudElement(RenderHud);
        REX::INFO("[F4MP] menu + HUD registrados");
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
