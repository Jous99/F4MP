#include "pch.h"
#include "F4SEMenuFramework.h"
#include "network/NetworkClient.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// HITO 2 + 3: menu in-game (F4SE Menu Framework) + red real.
// El menu conecta/desconecta y un hilo de red envia tu posicion al servidor.

namespace F4MP
{
    static char g_serverAddr[64] = "127.0.0.1";
    static int  g_serverPort = 7779;
    static char g_playerName[64] = "Wastelander";
    static std::atomic<bool> g_netThreadStarted{ false };

    static void BodiesSync();  // declaracion adelantada (se define mas abajo)

    // ---- Hilo de red: procesa mensajes, envia la posicion y sincroniza cuerpos ----
    static void NetThread()
    {
        using namespace std::chrono;
        auto lastSend = steady_clock::now();
        auto lastSync = steady_clock::now();

        for (;;) {
            auto& net = Network::NetworkClient::GetInstance();
            net.PumpMessages();

            if (net.IsConnected()) {
                auto now = steady_clock::now();
                if (duration_cast<milliseconds>(now - lastSend).count() >= 50) {  // 20/s
                    lastSend = now;
                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        const auto pos = player->GetPosition();
                        Network::PlayerPositionMsg msg{};
                        msg.playerId = net.GetPlayerId();
                        msg.x = pos.x;
                        msg.y = pos.y;
                        msg.z = pos.z;
                        msg.angleZ = player->GetEyeHeading();  // hacia donde miras (cursor/camara)
                        msg.cellId = 0;
                        net.SendPacket(Network::MessageType::PlayerPosition, &msg, sizeof(msg));
                    }
                }
                // Sincronizar cuerpos en el hilo PRINCIPAL ~60/s (para interpolar suave).
                if (duration_cast<milliseconds>(now - lastSync).count() >= 16) {
                    lastSync = now;
                    F4SE::GetTaskInterface()->AddTask([]() { BodiesSync(); });
                }
            }

            std::this_thread::sleep_for(milliseconds(8));
        }
    }

    // ---- PRUEBA de spawn: coloca un Protectron en tu posicion via Papyrus ----
    // Llama a ObjectReference.PlaceAtMe por la VM (independiente de version, sin
    // direcciones fijas). Es solo para comprobar que spawnear funciona.
    static const uint32_t kDummyBase = 0x00106B09;  // Protectron (prueba)

    // Busca un actor de la base indicada cerca del jugador que NO este ya reclamado.
    static RE::Actor* FindNearbyActor(uint32_t baseFormId, const std::unordered_set<uint32_t>& exclude)
    {
        auto* pl = RE::PlayerCharacter::GetSingleton();
        if (!pl) return nullptr;
        auto* cell = pl->GetParentCell();
        if (!cell) return nullptr;
        const RE::NiPoint3 pos = pl->GetPosition();

        RE::Actor* found = nullptr;
        cell->ForEachReferenceInRange(pos, 800.0f, [&](RE::TESObjectREFR* ref) {
            if (ref && ref != static_cast<RE::TESObjectREFR*>(pl)) {
                auto* base = ref->GetObjectReference();
                if (base && base->GetFormID() == baseFormId && exclude.find(ref->GetFormID()) == exclude.end()) {
                    found = ref->As<RE::Actor>();
                    return RE::BSContainer::ForEachResult::kStop;
                }
            }
            return RE::BSContainer::ForEachResult::kContinue;
        });
        return found;
    }

    // ================= Cuerpos de jugadores remotos (Fase A) =================
    static std::unordered_map<uint32_t, uint32_t> g_bodies;      // playerId -> formID del actor
    static bool g_spawnPending = false;
    static uint32_t g_spawnFor = 0;
    static std::chrono::steady_clock::time_point g_spawnAt;

    static std::unordered_set<uint32_t> ClaimedFormIds()
    {
        std::unordered_set<uint32_t> s;
        for (auto& [pid, fid] : g_bodies) s.insert(fid);
        return s;
    }

    // Se ejecuta en el hilo PRINCIPAL: crea/mueve un cuerpo por cada jugador remoto.
    static void BodiesSync()
    {
        auto& net = Network::NetworkClient::GetInstance();
        if (!net.IsConnected()) {
            // Desconectado: despawnear cualquier cuerpo que haya quedado.
            for (auto& [pid, fid] : g_bodies) {
                char cmd[64];
                std::snprintf(cmd, sizeof(cmd), "prid %x", fid);
                RE::Console::ExecuteCommand(cmd);
                RE::Console::ExecuteCommand("disable");
                RE::Console::ExecuteCommand("markfordelete");
            }
            if (!g_bodies.empty()) {
                REX::INFO("[F4MP] desconectado: {} cuerpo(s) despawneado(s)", g_bodies.size());
                g_bodies.clear();
            }
            return;
        }
        auto remotos = net.GetRemotePlayers();

        // dt real entre ticks (para que la suavidad no dependa de los FPS).
        static std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();
        auto ahora = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(ahora - lastTick).count();
        lastTick = ahora;
        if (dt <= 0.0f || dt > 0.5f) dt = 0.033f;  // clamp por si hubo pausa/carga

        // Suavizado exponencial: alcanza el objetivo en ~tau segundos.
        const float tau = 0.12f;
        float alpha = 1.0f - std::exp(-dt / tau);

        // 0. Limpiar cuerpos de jugadores que ya no estan conectados.
        std::vector<uint32_t> muertos;
        for (auto& [pid, fid] : g_bodies) {
            if (remotos.find(pid) == remotos.end()) muertos.push_back(pid);
        }
        for (uint32_t pid : muertos) {
            uint32_t fid = g_bodies[pid];
            char cmd[64];
            std::snprintf(cmd, sizeof(cmd), "prid %x", fid);
            RE::Console::ExecuteCommand(cmd);   // seleccionar la referencia
            RE::Console::ExecuteCommand("disable");
            RE::Console::ExecuteCommand("markfordelete");
            g_bodies.erase(pid);
            REX::INFO("[F4MP] cuerpo eliminado (jugador {} desconectado, actor {:#x})", pid, fid);
        }

        // 1. Mover suavemente los cuerpos hacia la posicion de su jugador.
        for (auto& [pid, fid] : g_bodies) {
            auto it = remotos.find(pid);
            if (it == remotos.end()) continue;
            auto* form = RE::TESForm::GetFormByID(fid);
            auto* actor = form ? form->As<RE::Actor>() : nullptr;
            if (!actor) continue;

            const RE::NiPoint3 objetivo{ it->second.x, it->second.y, it->second.z };
            const RE::NiPoint3 actual = actor->GetPosition();
            const RE::NiPoint3 delta = objetivo - actual;
            const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

            if (dist > 1500.0f) {
                // Salto grande (spawn inicial o fast travel): teletransportar.
                actor->SetPosition(objetivo, true);
            } else {
                // Interpolar hacia el objetivo.
                RE::NiPoint3 nueva{ actual.x + delta.x * alpha,
                                    actual.y + delta.y * alpha,
                                    actual.z + delta.z * alpha };
                actor->SetPosition(nueva, true);
            }

            // Rotacion: mirar hacia donde MIRA el jugador (heading de vista recibido).
            // SetHeading fija el valor; UpdateActor3DPosition lo aplica al modelo 3D.
            actor->SetHeading(it->second.angleZ);
            actor->UpdateActor3DPosition();

            // Log throttled (1/s) para depurar la orientacion.
            static std::chrono::steady_clock::time_point lastLog{};
            if (std::chrono::duration<float>(ahora - lastLog).count() > 1.0f) {
                lastLog = ahora;
                REX::INFO("[F4MP] jugador {} angleZ recibido={:.2f} rad, heading cuerpo={:.2f} rad",
                    pid, it->second.angleZ, actor->GetHeading());
            }
        }

        // 2. Resolver un spawn pendiente (el placeatme es asincrono).
        if (g_spawnPending) {
            if (std::chrono::duration<float>(std::chrono::steady_clock::now() - g_spawnAt).count() > 0.4f) {
                RE::Actor* actor = FindNearbyActor(kDummyBase, ClaimedFormIds());
                if (actor && remotos.find(g_spawnFor) != remotos.end()) {
                    g_bodies[g_spawnFor] = actor->GetFormID();
                    REX::INFO("[F4MP] cuerpo asignado a jugador {} (actor {:#x})", g_spawnFor, actor->GetFormID());
                }
                g_spawnPending = false;
            }
            return;  // un spawn a la vez
        }

        // 3. Iniciar un spawn para el primer jugador remoto sin cuerpo.
        for (auto& [pid, pos] : remotos) {
            if (g_bodies.find(pid) == g_bodies.end()) {
                RE::Console::ExecuteCommand("player.placeatme 00106B09 1");
                g_spawnPending = true;
                g_spawnFor = pid;
                g_spawnAt = std::chrono::steady_clock::now();
                REX::INFO("[F4MP] spawneando cuerpo para jugador {}", pid);
                break;
            }
        }
    }

    static void SpawnDummy()
    {
        // 1. Spawnear via consola (funciona de forma fiable).
        REX::INFO("[F4MP] spawn: placeatme");
        RE::Console::ExecuteCommand("player.placeatme 00106B09 1");

        // 2. El spawn es asincrono: esperar un poco y, en el hilo principal,
        //    localizar el actor y moverlo a un lado (prueba de spawn+find+move).
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
            F4SE::GetTaskInterface()->AddTask([]() {
                RE::Actor* actor = FindNearbyActor(kDummyBase, {});
                if (!actor) { REX::WARN("[F4MP] spawn: no encontre el actor spawneado"); return; }

                auto* pl = RE::PlayerCharacter::GetSingleton();
                RE::NiPoint3 target = pl ? pl->GetPosition() : RE::NiPoint3{};
                target.x += 300.0f;  // moverlo 300 unidades a un lado
                actor->SetPosition(target, true);
                REX::INFO("[F4MP] spawn: actor {:#x} encontrado y movido", actor->GetFormID());
            });
        }).detach();
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
