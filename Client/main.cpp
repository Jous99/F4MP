#include <windows.h>
#include "../Base/Logger.h"

// Forzamos que estas funciones sean visibles para F4SE
extern "C" {
    // 1. Identificación del plugin
    __declspec(dllexport) bool F4SEPlugin_Query(const void* f4se, void* info) {
        Logger::Log("F4MP: Query recibido.");
        return true;
    }

    // 2. Carga del plugin
    __declspec(dllexport) bool F4SEPlugin_Load(const void* f4se) {
        Logger::Log("F4MP: Cargando hilos...");
        
        // Creamos el hilo usando una función básica de Windows
        CreateThread(NULL, 0, [](LPVOID) -> DWORD {
            Logger::Log("F4MP: Hilo principal en ejecucion.");
            
            while (true) {
                if (GetAsyncKeyState(VK_END) & 0x8000) break;
                Sleep(100);
            }
            
            Logger::Log("F4MP: Hilo cerrado.");
            return 0;
        }, NULL, 0, NULL);

        return true;
    }
}

// Punto de entrada de la DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}