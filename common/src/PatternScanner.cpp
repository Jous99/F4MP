#include "f4mp/PatternScanner.h"
#include "f4mp/Logger.h"
#include <Windows.h>

namespace f4mp {

PatternScanner::PatternScanner()
    : m_moduleBase(0), m_moduleSize(0) {}

bool PatternScanner::Initialize(const std::string& moduleName) {
    HMODULE hModule = moduleName.empty() ? GetModuleHandle(nullptr) : GetModuleHandleA(moduleName.c_str());

    if (!hModule) {
        Logger::Error("PatternScanner: Failed to get module handle for '%s'", moduleName.c_str());
        return false;
    }

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(hModule);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        Logger::Error("PatternScanner: Invalid DOS header");
        return false;
    }

    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uintptr_t>(hModule) + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        Logger::Error("PatternScanner: Invalid NT header");
        return false;
    }

    m_moduleBase = reinterpret_cast<uintptr_t>(hModule);
    m_moduleSize = ntHeaders->OptionalHeader.SizeOfImage;

    Logger::Info("PatternScanner: Initialized module base: 0x%llX, size: 0x%zX", m_moduleBase, m_moduleSize);
    return true;
}

std::vector<PatternScanner::PatternByte> PatternScanner::ParsePattern(const std::string& pattern) const {
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

std::optional<uintptr_t> PatternScanner::FindPattern(const std::string& pattern) const {
    return FindPattern(ParsePattern(pattern));
}

std::optional<uintptr_t> PatternScanner::FindPattern(const std::vector<PatternByte>& pattern) const {
    if (m_moduleBase == 0 || pattern.empty()) {
        return std::nullopt;
    }

    const auto* scanBegin = reinterpret_cast<const uint8_t*>(m_moduleBase);
    const auto* scanEnd = scanBegin + m_moduleSize;

    for (const auto* current = scanBegin; current < scanEnd; ++current) {
        bool found = true;
        for (size_t i = 0; i < pattern.size(); ++i) {
            if (current + i >= scanEnd) {
                found = false;
                break;
            }
            if (!pattern[i].wildcard && static_cast<uint8_t>(current[i]) != pattern[i].value) {
                found = false;
                break;
            }
        }
        if (found) {
            return reinterpret_cast<uintptr_t>(current);
        }
    }

    return std::nullopt;
}

}
