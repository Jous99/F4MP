#ifndef F4MPCLIENT_PATTERNSCANNER_H
#define F4MPCLIENT_PATTERNSCANNER_H

#include <Windows.h>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <sstream>

namespace Memory {

class PatternScanner {
public:
    PatternScanner() : m_moduleBase(0), m_moduleSize(0) {}

    bool Initialize(const std::string& moduleName = "") {
        HMODULE hModule = moduleName.empty() ? GetModuleHandle(nullptr) : GetModuleHandleA(moduleName.c_str());
        if (!hModule) return false;

        auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(hModule);
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

        auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uintptr_t>(hModule) + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return false;

        m_moduleBase = reinterpret_cast<uintptr_t>(hModule);
        m_moduleSize = ntHeaders->OptionalHeader.SizeOfImage;
        return true;
    }

    std::optional<uintptr_t> FindPattern(const std::string& pattern) const {
        auto bytes = ParsePattern(pattern);
        if (bytes.empty() || m_moduleBase == 0) return std::nullopt;

        const auto* scanBegin = reinterpret_cast<const uint8_t*>(m_moduleBase);
        const auto* scanEnd = scanBegin + m_moduleSize;

        for (const auto* current = scanBegin; current < scanEnd; ++current) {
            bool found = true;
            for (size_t i = 0; i < bytes.size(); ++i) {
                if (current + i >= scanEnd) { found = false; break; }
                if (!bytes[i].wildcard && static_cast<uint8_t>(current[i]) != bytes[i].value) {
                    found = false;
                    break;
                }
            }
            if (found) return reinterpret_cast<uintptr_t>(current);
        }
        return std::nullopt;
    }

    uintptr_t GetModuleBase() const { return m_moduleBase; }
    size_t GetModuleSize() const { return m_moduleSize; }

private:
    struct PatternByte { uint8_t value; bool wildcard; };

    std::vector<PatternByte> ParsePattern(const std::string& pattern) const {
        std::vector<PatternByte> result;
        std::istringstream stream(pattern);
        std::string byte;
        while (stream >> byte) {
            if (byte == "?" || byte == "??") {
                result.push_back({0, true});
            } else {
                result.push_back({static_cast<uint8_t>(std::strtoul(byte.c_str(), nullptr, 16)), false});
            }
        }
        return result;
    }

    uintptr_t m_moduleBase;
    size_t m_moduleSize;
};

}

#endif
