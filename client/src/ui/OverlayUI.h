#pragma once

#include <imgui.h>
#include <d3d11.h>
#include <string>

namespace f4mp {

class OverlayUI {
public:
    static OverlayUI& GetInstance();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd);
    void Shutdown();

    void Render(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv);

    void SetMenuVisible(bool visible);
    bool IsMenuVisible() const;

    void SetConnected(bool connected);
    void SetPlayerCount(int current, int max);
    void SetPing(int pingMs);
    void AddChatMessage(const std::string& sender, const std::string& message);

private:
    OverlayUI() = default;

    void RenderMenu();
    void RenderChat();
    void RenderConnectionStatus();

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    HWND m_hwnd = nullptr;

    bool m_menuVisible = false;
    bool m_connected = false;
    int m_currentPlayers = 0;
    int m_maxPlayers = 0;
    int m_pingMs = 0;

    struct ChatEntry {
        std::string sender;
        std::string message;
        float timestamp;
    };
    ChatEntry m_chatHistory[100];
    int m_chatCount = 0;

    char m_chatInput[256] = "";
    bool m_chatFocused = false;
};

}
