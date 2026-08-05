#include "Global.h"

#include "DirectXHook.h"

#include <common/include/GamePtr.h>
#include <common/include/Utilities.h>
#include <common/include/Types.h>
#include <common/include/Hook.h>
#include <common/include/PatternScanner.h>

#include <Network/NetworkClient.h>

static Hooks::Hook<Hooks::CallConvention::cdecl_t, void, const char*, va_list> printHook;
static uintptr_t printAddr = 0;

class ConsoleManager
{
public:
    MEMBER_FN_PREFIX(ConsoleManager);
    DEFINE_MEMBER_FN(VPrint, void, 0, const char* fmt, va_list args);
    DEFINE_MEMBER_FN(Print, void, 0, const char* str);
};

static ConsoleManager* g_console = nullptr;
static Types::UInt32* g_consoleHandle = nullptr;

static bool ResolveGameAddresses() {
    Memory::PatternScanner scanner;
    if (!scanner.Initialize()) {
        spdlog::error("[F4MP] Failed to initialize pattern scanner");
        return false;
    }

    spdlog::info("[F4MP] Pattern scanning for game addresses...");
    spdlog::info("[F4MP] Module base: {:#x}", (uintptr_t)scanner.GetModuleBase());

    auto consoleManagerResult = scanner.FindPattern("48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 40 ? C3");
    if (consoleManagerResult) {
        auto relAddr = *consoleManagerResult + 3;
        auto offset = *reinterpret_cast<int32_t*>(relAddr);
        g_console = *reinterpret_cast<ConsoleManager**>(relAddr + 4 + offset);
        spdlog::info("[F4MP] ConsoleManager resolved: {:#x}", reinterpret_cast<uintptr_t>(g_console));
    } else {
        spdlog::warn("[F4MP] ConsoleManager pattern not found");
        return false;
    }

    auto consolePrintResult = scanner.FindPattern("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 48 8B FA");
    if (consolePrintResult) {
        printAddr = *consolePrintResult;
        spdlog::info("[F4MP] Console::Print resolved: {:#x}", (uintptr_t)printAddr);
    } else {
        spdlog::warn("[F4MP] Console::Print pattern not found");
        return false;
    }

    return true;
}

void Console_Print(const char* fmt, ...) {
    if (g_console) {
        va_list args;
        va_start(args, fmt);
        CALL_MEMBER_FN(g_console, VPrint)(fmt, args);
        va_end(args);
    }
}

#include <atomic>
static std::atomic<bool> g_initStarted{ false };

DWORD WINAPI Main(LPVOID lpThreadParameter) {
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    auto console = spdlog::stdout_color_mt("console");

    // Logger SINCRONO con volcado inmediato: asi 'logs/f4mp.txt' sobrevive
    // aunque el juego crashee (con el asincrono se perdian las ultimas lineas).
    auto file = spdlog::basic_logger_mt("f4mp_logger", "logs/f4mp.txt");
    file->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(file);

    spdlog::get("console")->info("F4MP Console Loaded");
    spdlog::info("[F4MP] === init: inicio ===");

    try {
        spdlog::info("[F4MP] init: resolviendo direcciones del juego (pattern scan)...");
        if (!ResolveGameAddresses()) {
            spdlog::warn("[F4MP] Algunas direcciones NO se resolvieron (patrones no encontrados).");
        }

        spdlog::info("[F4MP] init: enganchando DirectX (D3D11 Present)...");
        Hooks::DirectX::Init();
        spdlog::info("[F4MP] init: DirectX OK");

        // Hook de la consola del juego: DESACTIVADO. Enganchaba una funcion cuya
        // firma no coincide y llenaba la salida de basura binaria. No es necesario.
        // (printAddr se resuelve pero no lo usamos por ahora.)

        spdlog::info("[F4MP] init: iniciando red...");
        Network::NetworkClient::GetInstance().Initialize();

        spdlog::info("[F4MP] === init: completado ===");
    } catch (const std::exception& e) {
        spdlog::error("[F4MP] EXCEPCION en init: {}", e.what());
    } catch (...) {
        spdlog::error("[F4MP] EXCEPCION desconocida en init");
    }

    return TRUE;
}

// Arranca la inicializacion de F4MP una sola vez (idempotente).
// La llama el manejador de mensajes de F4SE (F4SEPlugin.cpp) cuando el juego
// esta listo y, como respaldo para inyeccion/ASI, el watchdog de abajo.
extern "C" void F4MP_StartInit() {
    bool expected = false;
    if (!g_initStarted.compare_exchange_strong(expected, true)) return; // ya iniciado
    if (auto handle = CreateThread(nullptr, 0, Main, nullptr, 0, nullptr)) {
        CloseHandle(handle);
    }
}

BOOL WINAPI Detach() {
    Network::NetworkClient::GetInstance().Shutdown();
    return TRUE;
}

// Respaldo para carga por inyeccion / ASI loader (sin F4SE):
// tras un retardo, si nadie ha iniciado aun, iniciamos nosotros.
static DWORD WINAPI InitWatchdog(LPVOID) {
    Sleep(15000);
    F4MP_StartInit();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved) {

    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        // IMPORTANTE: no inicializamos nada pesado aqui (no es seguro bajo F4SE,
        // que carga los plugins muy pronto). Bajo F4SE, el arranque se dispara
        // al recibir el mensaje de "juego listo" (ver F4SEPlugin.cpp).
        // Este watchdog solo cubre el caso de inyeccion/ASI sin F4SE.
        if (auto handle = CreateThread(nullptr, 0, InitWatchdog, nullptr, 0, nullptr)) {
            CloseHandle(handle);
        }
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH && !lpReserved) {
        return Detach();
    }

    return TRUE;
}
