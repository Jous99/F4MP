#include "f4se/PluginAPI.h"
#include "f4se_common/f4se_version.h"
#include "f4se_common/SafeWrite.h"
#include "f4mp/Logger.h"
#include "f4mp/Config.h"
#include "f4mp/PatternScanner.h"
#include "GameAddresses.h"

#include <Windows.h>
#include <shlobj.h>
#include <string>

PluginHandle g_pluginHandle = kPluginHandle_Invalid;
static F4SEPapyrusInterface* g_papyrus = nullptr;
static F4SEMessagingInterface* g_messaging = nullptr;
static F4SETrampolineInterface* g_trampoline = nullptr;

static void F4SEMessageHandler(F4SEMessagingInterface::Message* msg) {
    switch (msg->type) {
        case F4SEMessagingInterface::kMessage_PostLoad:
            f4mp::Logger::Info("F4SE PostLoad message received");
            break;

        case F4SEMessagingInterface::kMessage_PostPostLoad:
            f4mp::Logger::Info("F4SE PostPostLoad message received");
            break;

        case F4SEMessagingInterface::kMessage_InputLoaded:
            f4mp::Logger::Info("Input loaded - initializing F4MP");
            break;

        case F4SEMessagingInterface::kMessage_NewGame:
            f4mp::Logger::Info("New game started");
            break;

        case F4SEMessagingInterface::kMessage_GameLoaded:
            f4mp::Logger::Info("Game loaded");
            break;

        case F4SEMessagingInterface::kMessage_GameDataReady:
            f4mp::Logger::Info("Game data ready");
            break;
    }
}

static bool RegisterPapyrusFunctions(VirtualMachine* vm) {
    f4mp::Logger::Info("Registering Papyrus functions");
    return true;
}

static bool IsCompatible(const F4SEInterface* f4se) {
    if (f4se->isEditor) {
        f4mp::Logger::Error("Plugin is not compatible with the editor");
        return false;
    }

    uint32_t runtimeVersion = f4se->runtimeVersion;
    f4mp::Logger::Info("Runtime version: 0x%08X", runtimeVersion);

    uint32_t minVersion = RUNTIME_VERSION_1_10_980;
    if (runtimeVersion < minVersion) {
        f4mp::Logger::Error("Runtime version too old. Minimum: 1.10.980");
        return false;
    }

    return true;
}

extern "C" {

__declspec(dllexport) F4SEPluginVersionData F4SEPlugin_Version = {
    F4SEPluginVersionData::kVersion,

    1,
    "F4MP",
    "F4MP Team",

    0,
    0,
    { RUNTIME_VERSION_1_10_980, 0 },

    0,

    0,
    0,
    {0}
};

__declspec(dllexport) bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info) {
    std::string logPath = "C:\\Users\\Jous\\Documents\\My Games\\Fallout4\\F4SE\\F4MP.log";

    try {
        f4mp::Logger::Initialize(logPath, true);
    } catch (...) {
        return false;
    }

    f4mp::Logger::Info("F4MP v1.0.0 - Querying");
    f4mp::Logger::Info("F4SE version: 0x%08X", f4se->f4seVersion);
    f4mp::Logger::Info("Runtime version: 0x%08X", f4se->runtimeVersion);

    info->infoVersion = PluginInfo::kInfoVersion;
    info->name = "F4MP";
    info->version = 1;

    if (!IsCompatible(f4se)) {
        f4mp::Logger::Error("Incompatible runtime version");
        return false;
    }

    f4mp::Logger::Info("Query successful");
    return true;
}

__declspec(dllexport) bool F4SEPlugin_Load(const F4SEInterface* f4se) {
    f4mp::Logger::Info("Loading F4MP plugin");

    if (!IsCompatible(f4se)) {
        f4mp::Logger::Error("Incompatible runtime version");
        return false;
    }

    g_pluginHandle = f4se->GetPluginHandle();

    g_papyrus = (F4SEPapyrusInterface*)f4se->QueryInterface(kInterface_Papyrus);
    if (!g_papyrus) {
        f4mp::Logger::Error("Failed to get Papyrus interface");
        return false;
    }

    if (!g_papyrus->Register(RegisterPapyrusFunctions)) {
        f4mp::Logger::Error("Failed to register Papyrus functions");
        return false;
    }
    f4mp::Logger::Info("Papyrus functions registered");

    g_messaging = (F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging);
    if (g_messaging) {
        if (g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler)) {
            f4mp::Logger::Info("Messaging listener registered");
        }
    }

    g_trampoline = (F4SETrampolineInterface*)f4se->QueryInterface(kInterface_Trampoline);
    if (g_trampoline) {
        f4mp::Logger::Info("Trampoline interface available");
    }

    f4mp::Config::GetInstance().Load();

    if (!f4mp::GameAddresses::GetInstance().Resolve()) {
        f4mp::Logger::Warn("Some game addresses could not be resolved");
    }

    f4mp::Logger::Info("F4MP loaded successfully");
    return true;
}

}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            break;
        case DLL_PROCESS_DETACH:
            f4mp::Logger::Shutdown();
            break;
    }
    return TRUE;
}
