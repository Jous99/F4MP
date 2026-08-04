// Port de F4MP como plugin de F4SE (Fallout 4 Script Extender).
//
// Un plugin de F4SE bien hecho NO inicializa cosas pesadas al cargar el DLL.
// En vez de eso, se registra en la mensajeria de F4SE y arranca cuando el juego
// avisa de que ya esta listo (kMessage_GameDataReady / kMessage_GameLoaded).
// Esto evita el crash "exception occurred while loading plugins".
//
// Aqui se declara de forma minima la API de F4SE (sin necesitar el SDK completo).

#include <cstdint>

// La inicializacion real de F4MP vive en main.cpp.
extern "C" void F4MP_StartInit();

// ---------------------------------------------------------------------------
//  Estructuras minimas de la API de F4SE
// ---------------------------------------------------------------------------
typedef uint32_t PluginHandle;

struct F4SEInterface {
    uint32_t     f4seVersion;
    uint32_t     runtimeVersion;
    uint32_t     interfaceVersion;
    void*        (*QueryInterface)(uint32_t id);
    PluginHandle (*GetPluginHandle)(void);
    uint32_t     (*GetReleaseIndex)(void);
    const void*  (*GetPluginInfo)(const char* name);
};

struct F4SEMessagingInterface {
    struct Message {
        const char* sender;
        uint32_t    type;
        uint32_t    dataLen;
        void*       data;
    };
    typedef void (*EventCallback)(Message* msg);

    uint32_t interfaceVersion;
    bool  (*RegisterListener)(PluginHandle listener, const char* sender, EventCallback handler);
    bool  (*Dispatch)(PluginHandle sender, uint32_t messageType, void* data, uint32_t dataLen, const char* receiver);
    void* (*GetEventDispatcher)(uint32_t dispatcherId);
};

// IDs de interfaz de F4SE
enum { kInterface_Messaging = 1 };

// Tipos de mensaje de F4SE (en orden)
enum {
    kMessage_PostLoad = 0,
    kMessage_PostPostLoad,
    kMessage_PreLoadGame,
    kMessage_PostLoadGame,
    kMessage_PreSaveGame,
    kMessage_PostSaveGame,
    kMessage_DeleteGame,
    kMessage_InputLoaded,
    kMessage_NewGame,
    kMessage_GameLoaded,
    kMessage_GameDataReady,
};

// ---------------------------------------------------------------------------
//  Estructura de version exportada (API de F4SE Next-Gen / Anniversary)
// ---------------------------------------------------------------------------
struct F4SEPluginVersionData {
    enum { kVersion = 1 };
    enum {
        kVersionIndependent_AddressLibraryPostNG = 1 << 0,
        kVersionIndependent_Signatures           = 1 << 1,
        kVersionIndependent_StructsPost1105      = 1 << 2,
    };
    enum { kVersionIndependentEx_NoStructUse = 1 << 0 };

    uint32_t dataVersion;
    uint32_t pluginVersion;
    char     name[256];
    char     author[256];
    char     supportEmail[256];
    uint32_t versionIndependenceEx;
    uint32_t versionIndependence;
    uint32_t compatibleVersions[16];
    uint32_t seVersionRequired;
};

extern "C" __declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version =
{
    F4SEPluginVersionData::kVersion,
    1,          // pluginVersion
    "F4MP",     // name
    "F4MP",     // author
    "",         // supportEmail
    // Independiente de version: F4MP usa pattern scanning.
    // Ponemos el flag "Signatures" en LOS DOS campos por si F4SE espera el
    // orden contrario; asi cualquiera de los dos vale como "independiente".
    F4SEPluginVersionData::kVersionIndependent_Signatures | F4SEPluginVersionData::kVersionIndependent_AddressLibraryPostNG,
    F4SEPluginVersionData::kVersionIndependent_Signatures | F4SEPluginVersionData::kVersionIndependent_AddressLibraryPostNG,
    { 0 },      // compatibleVersions: todas
    0           // seVersionRequired: cualquiera
};

// ---------------------------------------------------------------------------
//  Manejador de mensajes de F4SE
// ---------------------------------------------------------------------------
static void F4MP_OnF4SEMessage(F4SEMessagingInterface::Message* msg) {
    if (!msg) return;
    // Cuando el juego esta cargado / los datos listos, arrancamos F4MP.
    if (msg->type == kMessage_GameDataReady || msg->type == kMessage_GameLoaded) {
        F4MP_StartInit();
    }
}

// ---------------------------------------------------------------------------
//  F4SE llama a esto tras validar la version. Aqui nos registramos.
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface* f4se) {
    PluginHandle handle = f4se->GetPluginHandle();

    auto* messaging = reinterpret_cast<F4SEMessagingInterface*>(
        f4se->QueryInterface(kInterface_Messaging));

    if (messaging) {
        messaging->RegisterListener(handle, "F4SE", F4MP_OnF4SEMessage);
    } else {
        // Sin mensajeria disponible: arrancamos directamente (menos ideal).
        F4MP_StartInit();
    }
    return true;
}
