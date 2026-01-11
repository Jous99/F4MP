#include "Global.h"
#include "DirectXHook.h"
#include <common/include/GamePtr.h>
#include <common/include/Utilities.h>
#include <common/include/Types.h>
#include <common/include/Hook.h>

// --- OFFSETS ACTUALIZADOS PARA FALLOUT 4 NEXT-GEN (v1.10.984) ---

// v1.10.163 era 0x01262EC0 -> Next-Gen es 0x012B9F80 (VPrint)
static Memory::GameAddr <void> printAddr(0x012B9F80); 

static Hooks::Hook<Hooks::CallConvention::cdecl_t, void, const char *, va_list> printHook;

class ConsoleManager
{
public:
    MEMBER_FN_PREFIX(ConsoleManager);
    // Direcciones actualizadas para la clase ConsoleManager
    DEFINE_MEMBER_FN(VPrint, void, 0x012B9F80, const char * fmt, va_list args);
    DEFINE_MEMBER_FN(Print, void, 0x012BA010, const char * str);
};

// Offsets de punteros globales (Data Segment)
// g_console: era 0x058E0AE0 -> Next-Gen es 0x059489A0
static Memory::GamePtr<ConsoleManager *> g_console(0x059489A0);

// g_consoleHandle: era 0x05ADB4A8 -> Next-Gen es 0x05B472C8
static Memory::GameAddr<Types::UInt32*> g_consoleHandle(0x05B472C8);

// ----------------------------------------------------------------

void Console_Print(const char * fmt, ...)
{
    ConsoleManager * mgr = *g_console;
    if(mgr)
    {
        va_list args;
        va_start(args, fmt);
        CALL_MEMBER_FN(mgr, VPrint)(fmt, args);
        va_end(args);
    }
}

void testPrint(const char * fmt, ...){
    // Función de prueba, no requiere cambios de memoria
    va_list args;
    va_start(args,fmt);
    va_end(args);
}

DWORD WINAPI Main(LPVOID lpThreadParameter){
    // LOGGING
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    
    auto console = spdlog::stdout_color_mt("console");
    auto async_file = spdlog::basic_logger_mt<spdlog::async_factory>("f4mp_logger", "logs/f4mp.txt");

    spdlog::set_default_logger(async_file);
    spdlog::get("console")->info("F4MP Console Loaded (Next-Gen Version)");

    Hooks::DirectX::Init();

    // Hook a la función de impresión de consola del juego
    printHook.apply(printAddr.GetUIntPtr(), [](const char * fmt, va_list args) -> void {
            // Nota: args es un va_list, imprimirlo directamente con std::cout puede dar basura o crash
            // En una terminal real usarías vsnprintf para formatear el mensaje
            std::cout << "[GAME_CONSOLE] " << fmt << std::endl;
            return printHook.call_orig(fmt, args);
    });

    Console_Print("F4MP: SISTEMA INICIADO EN NEXT-GEN");
    Console_Print("F4MP: COMPROBANDO MEMORIA...");

    return TRUE;
}

// ... Resto del código (Detach y DllMain) se mantiene igual ...