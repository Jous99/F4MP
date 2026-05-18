#include "OverlayUI.h"
#include "f4mp/Logger.h"

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <chrono>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace f4mp {

OverlayUI& OverlayUI::GetInstance() {
    static OverlayUI instance;
    return instance;
}

bool OverlayUI::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd) {
    m_device = device;
    m_context = context;
    m_hwnd = hwnd;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(hwnd)) {
        Logger::Error("Failed to initialize ImGui Win32 backend");
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, context)) {
        Logger::Error("Failed to initialize ImGui DX11 backend");
        return false;
    }

    Logger::Info("Overlay UI initialized");
    return true;
}

void OverlayUI::Shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    Logger::Info("Overlay UI shutdown");
}

void OverlayUI::Render(ID3D11DeviceContext* context, ID3D11RenderTargetView* rtv) {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    RenderConnectionStatus();

    if (m_menuVisible) {
        RenderMenu();
    }

    RenderChat();

    ImGui::Render();
    context->OMSetRenderTargets(1, &rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void OverlayUI::SetMenuVisible(bool visible) {
    m_menuVisible = visible;
    ImGui::GetIO().MouseDrawCursor = visible;
}

bool OverlayUI::IsMenuVisible() const {
    return m_menuVisible;
}

void OverlayUI::SetConnected(bool connected) {
    m_connected = connected;
}

void OverlayUI::SetPlayerCount(int current, int max) {
    m_currentPlayers = current;
    m_maxPlayers = max;
}

void OverlayUI::SetPing(int pingMs) {
    m_pingMs = pingMs;
}

void OverlayUI::AddChatMessage(const std::string& sender, const std::string& message) {
    if (m_chatCount >= 100) {
        for (int i = 0; i < 99; i++) {
            m_chatHistory[i] = m_chatHistory[i + 1];
        }
        m_chatCount = 99;
    }

    m_chatHistory[m_chatCount].sender = sender;
    m_chatHistory[m_chatCount].message = message;
    m_chatHistory[m_chatCount].timestamp = ImGui::GetTime();
    m_chatCount++;
}

void OverlayUI::RenderConnectionStatus() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(200, 60));
    ImGui::Begin("##Status", nullptr, flags);

    if (m_connected) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Connected");
        ImGui::Text("Players: %d/%d", m_currentPlayers, m_maxPlayers);
        ImGui::Text("Ping: %d ms", m_pingMs);
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Disconnected");
    }

    ImGui::End();
}

void OverlayUI::RenderMenu() {
    ImGui::Begin("F4MP Menu", &m_menuVisible);

    if (ImGui::CollapsingHeader("Connection")) {
        static char serverAddr[64] = "127.0.0.1";
        static int serverPort = 7779;

        ImGui::InputText("Server Address", serverAddr, sizeof(serverAddr));
        ImGui::InputInt("Server Port", &serverPort);

        if (ImGui::Button("Connect")) {
            Logger::Info("Connecting to %s:%d", serverAddr, serverPort);
        }
        ImGui::SameLine();
        if (ImGui::Button("Disconnect")) {
            Logger::Info("Disconnecting");
        }
    }

    if (ImGui::CollapsingHeader("Players")) {
        ImGui::Text("Online: %d/%d", m_currentPlayers, m_maxPlayers);
    }

    if (ImGui::CollapsingHeader("Settings")) {
        static bool debugLog = false;
        ImGui::Checkbox("Debug Logging", &debugLog);
    }

    ImGui::End();
}

void OverlayUI::RenderChat() {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 180));
    ImGui::SetNextWindowSize(ImVec2(400, 170));
    ImGui::Begin("##Chat", nullptr, flags);

    for (int i = 0; i < m_chatCount; i++) {
        ImGui::Text("[%s]: %s", m_chatHistory[i].sender.c_str(), m_chatHistory[i].message.c_str());
    }

    ImGui::InputText("##ChatInput", m_chatInput, sizeof(m_chatInput), ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::IsItemFocused()) {
        m_chatFocused = true;
    } else {
        m_chatFocused = false;
    }

    ImGui::End();
}

}
