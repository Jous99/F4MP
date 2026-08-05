#include "PlayerSync.h"

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <spdlog/spdlog.h>

#include <common/include/PatternScanner.h>
#include <Network/NetworkClient.h>

namespace PlayerSync {

static uintptr_t g_playerGlobal = 0;   // direccion del puntero global al jugador
static bool g_resolved = false;
static bool g_triedResolve = false;
static int  g_diagFrames = 0;
static std::chrono::steady_clock::time_point g_lastSend;

// Lectura de memoria PROTEGIDA: si la direccion no es valida, no crashea el
// juego; devuelve false. Esto es clave mientras afinamos los offsets.
static bool SafeRead(void* dst, uintptr_t src, size_t size) {
    __try {
        memcpy(dst, reinterpret_cast<const void*>(src), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static void ResolvePlayer() {
    g_triedResolve = true;

    Memory::PatternScanner scanner;
    if (!scanner.Initialize()) {
        spdlog::warn("[Sync] no se pudo iniciar el pattern scanner");
        return;
    }

    // CANDIDATO (a ajustar): patron para cargar el puntero global del jugador.
    // Muchas funciones hacen algo como:  mov rax, [rip+global] ; mov rcx,[rax+off] ...
    auto res = scanner.FindPattern("48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? 48 8B 01");
    if (!res) {
        spdlog::warn("[Sync] patron del jugador NO encontrado (hay que ajustar el patron)");
        return;
    }

    uintptr_t rel = *res + 3;                         // saltamos '48 8B 05'
    int32_t   off = *reinterpret_cast<int32_t*>(rel); // desplazamiento RIP-relative
    g_playerGlobal = rel + 4 + off;                   // direccion del PlayerCharacter** global
    g_resolved = true;
    spdlog::info("[Sync] puntero global del jugador @ {:#x}", g_playerGlobal);
}

void Update() {
    if (!g_triedResolve) ResolvePlayer();
    if (!g_resolved) return;

    // global -> PlayerCharacter*
    uintptr_t player = 0;
    if (!SafeRead(&player, g_playerGlobal, sizeof(player)) || player == 0) return;

    // --- DIAGNOSTICO ---
    // Cada 2 segundos volcamos floats en un rango de offsets para localizar la
    // posicion. Quedate quieto en un punto, abre la consola (~) y escribe:
    //   player.getpos x   (tambien y, z)
    // y compara con el ULTIMO bloque [Sync] del log para saber que offset es.
    {
        static std::chrono::steady_clock::time_point lastDiag;
        auto tnow = std::chrono::steady_clock::now();
        if (std::chrono::duration<float>(tnow - lastDiag).count() >= 2.0f) {
            lastDiag = tnow;

            // Validacion: en offset 0 de un objeto real hay un puntero (vtable).
            uintptr_t vtbl = 0;
            SafeRead(&vtbl, player, sizeof(vtbl));
            spdlog::info("[Sync] --- volcado --- player @ {:#x} vtable @ {:#x}", player, vtbl);

            // Escaneo MUY amplio; solo floats con magnitud de coordenada de mundo
            // (cientos a cientos de miles), para localizar la posicion.
            for (uintptr_t off = 0x00; off <= 0x1000; off += 4) {
                float f = 0.0f;
                if (SafeRead(&f, player + off, sizeof(f))) {
                    float a = f < 0.0f ? -f : f;
                    if (a >= 100.0f && a <= 500000.0f)
                        spdlog::info("[Sync]   offset {:#x} = {}", off, f);
                }
            }
        }
    }

    // --- ENVIO DE POSICION ---
    // TUNE: ajusta POS_OFFSET al que resulte ser la posicion (segun el volcado).
    static const uintptr_t POS_OFFSET = 0x54; // provisional
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    SafeRead(pos, player + POS_OFFSET, sizeof(pos));

    auto& net = Network::NetworkClient::GetInstance();
    if (!net.IsConnected()) return;

    // Enviamos a 10 veces por segundo como maximo.
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - g_lastSend).count() < 0.1f) return;
    g_lastSend = now;

    Network::PlayerPositionMsg msg{};
    msg.playerId = net.GetPlayerId();
    msg.x = pos[0];
    msg.y = pos[1];
    msg.z = pos[2];
    msg.cellId = 0;
    net.SendPacket(Network::MessageType::PlayerPosition, &msg, sizeof(msg));
}

}
