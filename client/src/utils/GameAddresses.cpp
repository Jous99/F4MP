#include "GameAddresses.h"
#include "f4mp/PatternScanner.h"
#include "f4mp/Logger.h"

#include <Windows.h>

namespace f4mp {

GameAddresses& GameAddresses::GetInstance() {
    static GameAddresses instance;
    return instance;
}

bool GameAddresses::Resolve() {
    PatternScanner scanner;
    if (!scanner.Initialize()) {
        Logger::Error("Failed to initialize pattern scanner");
        return false;
    }

    Logger::Info("Resolving game addresses...");
    Logger::Info("Module base: 0x%llX", scanner.GetModuleBase());

    auto consoleManager = scanner.FindPattern("48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 40 ? C3");
    if (consoleManager) {
        m_addresses.consoleManager = *consoleManager;
        Logger::Info("ConsoleManager: 0x%llX", m_addresses.consoleManager);
    } else {
        Logger::Warn("ConsoleManager pattern not found");
    }

    auto playerCharacter = scanner.FindPattern("48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 40 ? C3");
    if (playerCharacter) {
        m_addresses.playerCharacter = *playerCharacter;
        Logger::Info("PlayerCharacter: 0x%llX", m_addresses.playerCharacter);
    } else {
        Logger::Warn("PlayerCharacter pattern not found");
    }

    auto uiManager = scanner.FindPattern("48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B C8");
    if (uiManager) {
        m_addresses.uiManager = *uiManager;
        Logger::Info("UIManager: 0x%llX", m_addresses.uiManager);
    } else {
        Logger::Warn("UIManager pattern not found");
    }

    auto dataHandler = scanner.FindPattern("48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 40 ? C3");
    if (dataHandler) {
        m_addresses.dataHandler = *dataHandler;
        Logger::Info("DataHandler: 0x%llX", m_addresses.dataHandler);
    } else {
        Logger::Warn("DataHandler pattern not found");
    }

    m_resolved = true;
    Logger::Info("Game address resolution complete");
    return true;
}

}
