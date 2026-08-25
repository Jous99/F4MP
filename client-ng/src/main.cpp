#include "pch.h"
#include "F4SEMenuFramework.h"
#include "network/NetworkClient.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
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

    // Captura la apariencia del jugador local (Fase 2A) para enviarla al servidor.
    static Network::PlayerAppearanceMsg CaptureAppearance(uint32_t myId)
    {
        Network::PlayerAppearanceMsg a{};
        a.playerId = myId;
        a.sex = -1;
        auto* pl = RE::PlayerCharacter::GetSingleton();
        if (!pl) return a;
        a.sex = static_cast<int32_t>(pl->GetSex());
        if (pl->race) a.raceFormID = pl->race->GetFormID();

        auto* npc = pl->GetNPC();
        if (npc) {
            a.skinR = static_cast<uint8_t>(npc->bodyTintColorR);
            a.skinG = static_cast<uint8_t>(npc->bodyTintColorG);
            a.skinB = static_cast<uint8_t>(npc->bodyTintColorB);
            a.skinA = static_cast<uint8_t>(npc->bodyTintColorA);
            if (npc->headRelatedData && npc->headRelatedData->hairColor)
                a.hairColor = npc->headRelatedData->hairColor->color;

            auto hp = npc->GetHeadParts();
            uint8_t n = static_cast<uint8_t>(std::min<size_t>(hp.size(), 16));
            a.numHeadParts = n;
            for (uint8_t i = 0; i < n; ++i)
                a.headParts[i] = hp[i] ? hp[i]->GetFormID() : 0;
        }
        return a;
    }

    // ---- Hilo de red: procesa mensajes, envia la posicion y sincroniza cuerpos ----
    static void NetThread()
    {
        using namespace std::chrono;
        auto lastSend = steady_clock::now();
        auto lastSync = steady_clock::now();

        for (;;) {
            auto& net = Network::NetworkClient::GetInstance();
            net.PumpMessages();

            static bool s_sentAppearance = false;
            if (net.IsConnected()) {
                auto now = steady_clock::now();

                // Enviar nuestra apariencia UNA vez al conectar (Fase 2A).
                if (!s_sentAppearance) {
                    F4SE::GetTaskInterface()->AddTask([]() {
                        auto& net2 = Network::NetworkClient::GetInstance();
                        if (!net2.IsConnected()) return;
                        auto ap = CaptureAppearance(net2.GetPlayerId());
                        net2.SendPacket(Network::MessageType::PlayerAppearance, &ap, sizeof(ap));
                        REX::INFO("[F4MP] apariencia enviada (sexo={} raza={:#x} headparts={})",
                            ap.sex, ap.raceFormID, ap.numHeadParts);
                    });
                    s_sentAppearance = true;
                }

                if (duration_cast<milliseconds>(now - lastSend).count() >= 33) {  // 30/s
                    static RE::NiPoint3 s_lastPos{};
                    static bool s_havePos = false;
                    static auto s_lastPosTime = now;
                    const float sendDt = std::chrono::duration<float>(now - s_lastPosTime).count();
                    s_lastPosTime = now;

                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        const auto pos = player->GetPosition();
                        const float facing = player->GetEyeHeading();

                        // Velocidad y direccion a partir del desplazamiento real.
                        float speed = 0.0f, moveDir = 0.0f;
                        if (s_havePos && sendDt > 0.0001f && sendDt < 1.0f) {
                            float dx = pos.x - s_lastPos.x, dy = pos.y - s_lastPos.y;
                            float horizDist = std::sqrt(dx * dx + dy * dy);
                            speed = horizDist / sendDt;
                            if (horizDist > 1.0f) moveDir = std::atan2(dx, dy) - facing;  // 0 = adelante
                        }
                        s_lastPos = pos;
                        s_havePos = true;

                        Network::PlayerPositionMsg msg{};
                        msg.playerId = net.GetPlayerId();
                        msg.x = pos.x;
                        msg.y = pos.y;
                        msg.z = pos.z;
                        msg.angleZ = facing;  // hacia donde miras (cursor/camara)
                        msg.speed = speed;
                        msg.moveDir = moveDir;
                        msg.isMoving = speed > 10.0f;
                        msg.isSprinting = speed > 300.0f;   // umbral aprox. de sprint
                        msg.isRunning = speed > 150.0f && !msg.isSprinting;
                        msg.isSneaking = player->IsSneaking();
                        msg.isJumping = false;              // TODO: detectar salto/aire
                        msg.cellId = 0;
                        lastSend = now;
                        net.SendPacket(Network::MessageType::PlayerPosition, &msg, sizeof(msg));
                    } else {
                        lastSend = now;
                    }
                }
                // Sincronizar cuerpos en el hilo PRINCIPAL ~60/s (para interpolar suave).
                if (duration_cast<milliseconds>(now - lastSync).count() >= 16) {
                    lastSync = now;
                    F4SE::GetTaskInterface()->AddTask([]() { BodiesSync(); });
                }
            } else {
                s_sentAppearance = false;  // al reconectar, reenviar la apariencia
            }

            std::this_thread::sleep_for(milliseconds(8));
        }
    }

    // ---- PRUEBA de spawn: coloca un Protectron en tu posicion via Papyrus ----
    // Llama a ObjectReference.PlaceAtMe por la VM (independiente de version, sin
    // direcciones fijas). Es solo para comprobar que spawnear funciona.
    // 0x00000007 = base del jugador (lleva tu apariencia). placeatme 7 => copia de tu personaje.
    // (Antes 0x00106B09 = Protectron de prueba.)
    static const uint32_t kDummyBase = 0x00000007;

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
    static std::unordered_map<uint32_t, float> g_lastHeading;    // playerId -> ultimo heading aplicado

    // Ultimo estado de movimiento aplicado a cada cuerpo. Sirve para disparar los
    // eventos del grafo (moveStart/moveStop, SprintStart/SprintStop) SOLO cuando el
    // estado cambia, en vez de cada tick (si no, el grafo no avanzaria nunca).
    struct BodyAnimState { bool moving = false; bool sprinting = false; bool init = false; float smoothSpeed = 0.0f; };
    static std::unordered_map<uint32_t, BodyAnimState> g_animState;
    static bool g_lastOkSpeed = false, g_lastOkDir = false;       // retorno de SetGraphVariable (debug)
    static bool g_graphDumped = false;                            // ya volcamos las variables del grafo?

    // Diagnostico: lista que variables del grafo de animacion EXISTEN en el cuerpo.
    // Asi dejamos de adivinar nombres y usamos los reales.
    static void DumpGraphVars(RE::Actor* actor)
    {
        static const char* kFloats[] = {
            "Speed", "Direction", "SpeedSampled", "TurnDelta", "Turn", "MovementSpeed",
            "SpeedRatio", "PitchDelta", "LeftRight", "ForwardBack", "walkSpeed",
            "vertVelocity", "AnimSpeed", "fMovementDirection", "MoveSpeed"
        };
        static const char* kBools[] = {
            "IsMoving", "bMoving", "IsSprinting", "bSprinting", "bIsSprinting", "IsSneaking",
            "IsRunning", "bIsMovementDriven", "bAnimationDriven", "IsNPC", "bWantToSprint",
            "IsSynced", "bIsSynced", "GraphControlsMovement", "IsFirstPerson", "bAllowRotation"
        };
        REX::INFO("[F4MP] ===== VOLCADO variables del grafo (existen) =====");
        for (auto* n : kFloats) {
            float o = 0.0f;
            if (actor->GetGraphVariableImplFloat(n, o))
                REX::INFO("[F4MP]   FLOAT {} = {:.3f}", n, o);
        }
        for (auto* n : kBools) {
            bool o = false;
            if (actor->GetGraphVariableImplBool(n, o))
                REX::INFO("[F4MP]   BOOL  {} = {}", n, o);
        }
        REX::INFO("[F4MP] ===== fin del volcado =====");
    }
    static bool g_spawnPending = false;
    static uint32_t g_spawnFor = 0;
    static std::chrono::steady_clock::time_point g_spawnAt;

    static std::unordered_set<uint32_t> ClaimedFormIds()
    {
        std::unordered_set<uint32_t> s;
        for (auto& [pid, fid] : g_bodies) s.insert(fid);
        return s;
    }

    // Borra un cuerpo por API (mas fiable que comandos de consola). Disable lo oculta
    // al instante; SetWantsDelete + MarkAsDeleted hacen que el juego lo elimine y NO lo
    // guarde en la partida (los actores de placeatme son persistentes; asi evitamos que
    // se acumulen "muñecos fantasma" en el save).
    static void DespawnBody(uint32_t fid)
    {
        auto* form = RE::TESForm::GetFormByID(fid);
        auto* ref = form ? form->As<RE::TESObjectREFR>() : nullptr;
        if (!ref) return;
        ref->Disable();
        ref->SetWantsDelete(true);
        ref->MarkAsDeleted();
    }

    // Se ejecuta en el hilo PRINCIPAL: crea/mueve un cuerpo por cada jugador remoto.
    static void BodiesSync()
    {
        auto& net = Network::NetworkClient::GetInstance();
        if (!net.IsConnected()) {
            // Desconectado: despawnear cualquier cuerpo que haya quedado.
            for (auto& [pid, fid] : g_bodies) {
                DespawnBody(fid);
            }
            if (!g_bodies.empty()) {
                REX::INFO("[F4MP] desconectado: {} cuerpo(s) despawneado(s)", g_bodies.size());
                g_bodies.clear();
            }
            g_lastHeading.clear();
            g_animState.clear();
            return;
        }
        auto remotos = net.GetRemotePlayers();

        // dt real entre ticks (para que la suavidad no dependa de los FPS).
        static std::chrono::steady_clock::time_point lastTick = std::chrono::steady_clock::now();
        auto ahora = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(ahora - lastTick).count();
        lastTick = ahora;
        if (dt <= 0.0f || dt > 0.5f) dt = 0.033f;  // clamp por si hubo pausa/carga

        // (dt se usa abajo para mover el cuerpo a la velocidad correcta con Actor::Move.)

        // 0. Limpiar cuerpos de jugadores que ya no estan conectados.
        std::vector<uint32_t> muertos;
        for (auto& [pid, fid] : g_bodies) {
            if (remotos.find(pid) == remotos.end()) muertos.push_back(pid);
        }
        for (uint32_t pid : muertos) {
            uint32_t fid = g_bodies[pid];
            DespawnBody(fid);   // borrado fiable por API (Disable + MarkAsDeleted)
            g_bodies.erase(pid);
            g_lastHeading.erase(pid);
            g_animState.erase(pid);
            REX::INFO("[F4MP] cuerpo eliminado (jugador {} desconectado, actor {:#x})", pid, fid);
        }

        // 1. Mover suavemente los cuerpos hacia la posicion de su jugador.
        for (auto& [pid, fid] : g_bodies) {
            auto it = remotos.find(pid);
            if (it == remotos.end()) continue;
            auto* form = RE::TESForm::GetFormByID(fid);
            auto* actor = form ? form->As<RE::Actor>() : nullptr;
            if (!actor) continue;

            // Una sola vez, cuando el grafo ya esta cargado, volcar sus variables.
            if (!g_graphDumped) {
                float probe = 0.0f;
                if (actor->GetGraphVariableImplFloat("Speed", probe)) {  // grafo listo
                    g_graphDumped = true;
                    DumpGraphVars(actor);
                }
            }

            const RE::NiPoint3 objetivo{ it->second.x, it->second.y, it->second.z };
            const RE::NiPoint3 actual = actor->GetPosition();
            const RE::NiPoint3 delta = objetivo - actual;
            const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);

            if (dist > 1500.0f) {
                // Teletransporte: al aparecer (spawn) o en viaje rapido (cuerpo lejisimos).
                actor->SetPosition(objetivo, true);
            } else {
                // Seguimiento EXACTO por posicion: el cuerpo queda sincronizado con el marcador
                // (la posicion real de red). Nos acercamos suave al objetivo cada tick.
                // OJO: descartamos Actor::Move porque mueve al cuerpo en SU direccion de vista
                // (siempre hacia delante), no en la del mundo -> se iba a otro lado.
                // NOTA: esto por si solo NO anima las piernas (el motor no ve al cuerpo
                // "moverse"); la animacion se resolvera aparte (inyectando velocidad al
                // controlador fisico) sin cambiar esta posicion exacta.
                const float alpha = 1.0f - std::exp(-dt / 0.12f);  // suavizado ~0.12 s
                RE::NiPoint3 nueva{ actual.x + delta.x * alpha,
                                    actual.y + delta.y * alpha,
                                    actual.z + delta.z * alpha };
                actor->SetPosition(nueva, true);
            }

            // Rotacion: mirar hacia donde MIRA el jugador. Antes se llamaba a
            // UpdateActor3DPosition al cambiar el heading, pero eso RE-POSA al actor y
            // REINICIA la animacion (por eso se entrecortaba sin parar al moverse). Con
            // SetHeading basta para orientarlo; el refresco visual llega con el SetPosition
            // de cada frame, sin machacar la animacion.
            actor->SetHeading(it->second.angleZ);

            // --- Animaciones ---
            const auto& rp = it->second;
            BodyAnimState& st = g_animState[pid];

            // 1) SUAVIZAR la velocidad. La que llega esta calculada frame a frame en el
            //    emisor y salta muchisimo: cruzaba los umbrales de andar/sprint arriba y
            //    abajo constantemente, disparando eventos sin parar -> la animacion se
            //    reiniciaba todo el rato. Filtro paso-bajo para estabilizarla.
            float rawSpeed = (rp.speed > 0.0f && rp.speed < 1000.0f) ? rp.speed : 0.0f;
            st.smoothSpeed = st.smoothSpeed * 0.85f + rawSpeed * 0.15f;

            g_lastOkSpeed = actor->SetGraphVariableFloat("Speed", st.smoothSpeed);
            g_lastOkDir   = actor->SetGraphVariableFloat("Direction", rp.moveDir);

            // 2) Estado con HISTERESIS: umbral distinto para ARRANCAR y para PARAR, para que
            //    no vibre en el limite. Se calcula sobre la velocidad ya suavizada.
            const bool wantMoving   = st.moving   ? (st.smoothSpeed >  8.0f) : (st.smoothSpeed >  30.0f);
            const bool wantSprint   = st.sprinting? (st.smoothSpeed > 250.0f): (st.smoothSpeed > 340.0f);

            actor->SetGraphVariableBool("IsSprinting", wantSprint);
            // Agachado: fijar el ESTADO real del actor (no solo la variable del grafo).
            if (actor->IsSneaking() != rp.isSneaking) {
                actor->SetSneaking(rp.isSneaking);
                actor->SetGraphVariableBool("IsSneaking", rp.isSneaking);
            }

            // 3) EVENTOS del grafo: SOLO cuando el estado (ya estable) cambia de verdad.
            if (!st.init) {
                st.moving = wantMoving;
                st.sprinting = wantSprint;
                st.init = true;
            } else {
                if (wantMoving != st.moving) {
                    const char* ev = wantMoving ? "moveStart" : "moveStop";
                    actor->NotifyAnimationGraphImpl(ev);
                    REX::INFO("[F4MP] anim jugador {}: evento '{}' (speed suavizada {:.0f})", pid, ev, st.smoothSpeed);
                    st.moving = wantMoving;
                }
                if (wantSprint != st.sprinting) {
                    const char* ev = wantSprint ? "SprintStart" : "SprintStop";
                    actor->NotifyAnimationGraphImpl(ev);
                    REX::INFO("[F4MP] anim jugador {}: evento '{}' (speed suavizada {:.0f})", pid, ev, st.smoothSpeed);
                    st.sprinting = wantSprint;
                }
            }

            // 4) INYECTAR VELOCIDAD al controlador fisico para que el grafo SOSTENGA la
            //    animacion. El grafo lee la velocidad del controlador para saber si anda/
            //    corre; como el cuerpo se coloca con SetPosition (no se mueve solo), sin
            //    esto la animacion arranca y se congela tras el primer paso. La velocidad
            //    va en espacio de MUNDO (por eso ya no ira "siempre hacia delante" como con
            //    Actor::Move) y en unidades de HAVOK (metros): convertimos desde unidades
            //    de juego. Se inyecta DESPUES del SetPosition para que no la pise.
            if (auto* proc = actor->currentProcess) {
                if (auto* mid = proc->middleHigh) {
                    if (auto* cc = mid->charController.get()) {
                        constexpr float HAVOK = 0.0142875f;              // juego -> havok
                        const float worldDir = rp.angleZ + rp.moveDir;   // direccion real en el mundo
                        const float vh = st.smoothSpeed * HAVOK;
                        RE::hkVector4f vel{ std::sin(worldDir) * vh, std::cos(worldDir) * vh, 0.0f, 0.0f };
                        cc->SetLinearVelocityImpl(vel);
                    }
                }
            }

            // Log throttled (1/s): mide el DESFASE cuerpo vs objetivo (para diagnosticar
            // la desincronizacion). Si 'desync' crece cuando el jugador se mueve, es que el
            // cuerpo no llega/se va; si el cuerpo va en direccion girada, se vera aqui.
            static std::chrono::steady_clock::time_point lastLog{};
            if (std::chrono::duration<float>(ahora - lastLog).count() > 1.0f) {
                lastLog = ahora;
                const RE::NiPoint3 bp = actor->GetPosition();
                const float desyncH = std::sqrt((objetivo.x - bp.x) * (objetivo.x - bp.x) +
                                                 (objetivo.y - bp.y) * (objetivo.y - bp.y));
                REX::INFO("[F4MP] jugador {} speed={:.0f} mov={} | cuerpo=({:.0f},{:.0f}) objetivo=({:.0f},{:.0f}) desfase={:.0f}",
                    pid, it->second.speed, it->second.isMoving, bp.x, bp.y, objetivo.x, objetivo.y, desyncH);
            }
        }

        // 2. Resolver un spawn pendiente (el placeatme es asincrono).
        if (g_spawnPending) {
            if (std::chrono::duration<float>(std::chrono::steady_clock::now() - g_spawnAt).count() > 0.4f) {
                RE::Actor* actor = FindNearbyActor(kDummyBase, ClaimedFormIds());
                if (actor && remotos.find(g_spawnFor) != remotos.end()) {
                    g_bodies[g_spawnFor] = actor->GetFormID();
                    // CALMAR LA IA del cuerpo: es un NPC y, si no, deambula por su cuenta y
                    // se va a otro sitio distinto del marcador. El "paquete de no hacer nada"
                    // hace que deje de moverse solo, pero sigue animandose cuando lo movemos
                    // nosotros con Actor::Move.
                    actor->InitiateDoNothingPackage();
                    actor->StopCombat();
                    REX::INFO("[F4MP] cuerpo asignado a jugador {} (actor {:#x}), IA calmada", g_spawnFor, actor->GetFormID());
                }
                g_spawnPending = false;
            }
            return;  // un spawn a la vez
        }

        // 3. Iniciar un spawn para el primer jugador remoto sin cuerpo.
        for (auto& [pid, pos] : remotos) {
            if (g_bodies.find(pid) == g_bodies.end()) {
                RE::Console::ExecuteCommand("player.placeatme 7 1");
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
        RE::Console::ExecuteCommand("player.placeatme 7 1");

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
