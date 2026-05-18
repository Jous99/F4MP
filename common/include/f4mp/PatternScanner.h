#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace f4mp {

class PatternScanner {
public:
    struct PatternByte {
        uint8_t value;
        bool wildcard;
    };

    PatternScanner();

    bool Initialize(const std::string& moduleName = "");

    std::optional<uintptr_t> FindPattern(const std::string& pattern) const;
    std::optional<uintptr_t> FindPattern(const std::vector<PatternByte>& pattern) const;

    template<typename T>
    T* GetOffset(uintptr_t base, int32_t offset) const {
        if (base == 0) return nullptr;
        return reinterpret_cast<T*>(base + offset);
    }

    uintptr_t GetModuleBase() const { return m_moduleBase; }
    size_t GetModuleSize() const { return m_moduleSize; }

private:
    std::vector<PatternByte> ParsePattern(const std::string& pattern) const;

    uintptr_t m_moduleBase;
    size_t m_moduleSize;
};

}
