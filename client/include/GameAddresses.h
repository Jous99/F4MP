#pragma once

#include <cstdint>

namespace f4mp {

struct AddressData {
    uintptr_t consoleManager;
    uintptr_t consolePrint;
    uintptr_t consoleVPrint;
    uintptr_t playerCharacter;
    uintptr_t worldSpace;
    uintptr_t mainSingleton;
    uintptr_t uiManager;
    uintptr_t scriptFactory;
    uintptr_t dataHandler;
};

class GameAddresses {
public:
    static GameAddresses& GetInstance();

    bool Resolve();

    const AddressData& Get() const { return m_addresses; }
    bool IsResolved() const { return m_resolved; }

private:
    GameAddresses() = default;

    AddressData m_addresses{};
    bool m_resolved = false;
};

}
