#include "Heartbeat.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <cstdlib>

#pragma comment(lib, "winhttp.lib")

namespace {
    std::string JsonEscape(const std::string& s) {
        std::string out;
        for (char c : s) {
            if (c == '"' || c == '\\') { out += '\\'; out += c; }
            else if (c == '\n' || c == '\r' || c == '\t') { /* omitir */ }
            else out += c;
        }
        return out;
    }
    std::wstring Widen(const std::string& s) { return std::wstring(s.begin(), s.end()); }
}

namespace Heartbeat {

void Send(const std::string& url, const std::string& name,
          uint16_t port, uint32_t players, uint32_t maxPlayers)
{
    if (url.empty()) return;

    // Parsear:  [http(s)://]host[:puerto]
    std::string u = url;
    bool https = false;
    if (u.rfind("https://", 0) == 0) { https = true; u = u.substr(8); }
    else if (u.rfind("http://", 0) == 0) { u = u.substr(7); }
    while (!u.empty() && u.back() == '/') u.pop_back();

    std::string host = u;
    INTERNET_PORT hostPort = https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    const auto colon = u.find(':');
    if (colon != std::string::npos) {
        host = u.substr(0, colon);
        hostPort = static_cast<INTERNET_PORT>(atoi(u.substr(colon + 1).c_str()));
    }

    const std::string body =
        "{\"name\":\"" + JsonEscape(name) + "\",\"port\":" + std::to_string(port) +
        ",\"players\":" + std::to_string(players) +
        ",\"maxPlayers\":" + std::to_string(maxPlayers) + "}";

    const std::wstring whost = Widen(host);

    HINTERNET hSession = WinHttpOpen(L"F4MPServer/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return;

    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), hostPort, 0);
    if (hConnect) {
        const DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hReq = WinHttpOpenRequest(hConnect, L"POST", L"/heartbeat",
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (hReq) {
            const wchar_t* headers = L"Content-Type: application/json";
            if (WinHttpSendRequest(hReq, headers, static_cast<DWORD>(-1),
                    (LPVOID)body.data(), static_cast<DWORD>(body.size()),
                    static_cast<DWORD>(body.size()), 0)) {
                WinHttpReceiveResponse(hReq, NULL);
            }
            WinHttpCloseHandle(hReq);
        }
        WinHttpCloseHandle(hConnect);
    }
    WinHttpCloseHandle(hSession);
}

}
