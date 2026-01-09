#include "f4se/PluginAPI.h"
#include <shlobj.h>

// Información del Mod
IDebugLog gLog;
PluginHandle g_pluginHandle = kPluginHandle_Invalid;
F4SEMessagingInterface* g_messaging = NULL;

extern "C" {
    // Esta función la llama F4SE al arrancar para ver si el mod es compatible
    bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info) {
        gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\F4MP_Project.log");

        // Datos del mod
        info->infoVersion = PluginInfo::kInfoVersion;
        info->name = "F4MP_Project";
        info->version = 1;

        if (f4se->runtimeVersion != RUNTIME_VERSION_1_10_163) {
            _MESSAGE("ERROR: Version de juego no compatible.");
            return false;
        }

        return true;
    }

    // Esta función se ejecuta cuando el mod se carga oficialmente
    bool F4SEPlugin_Load(const F4SEInterface* f4se) {
        _MESSAGE("F4MP: Protocolo de terminal cargado correctamente.");
        
        g_pluginHandle = f4se->GetPluginHandle();
        
        // Aquí es donde en el futuro "engancharemos" los datos
        return true;
    }
};