#include "pch.h"
#include "F4SEMenuFramework.h"

// HITO 3: menu in-game usando F4SE Menu Framework (render ImGui compartido,
// sin montar nuestro propio hook de DirectX).
//
// Requisitos en el juego:
//   - Instalar el mod "F4SE Menu Framework" (aporta el render del menu).
//   - Se abre en el juego con la tecla del framework (por defecto  ]  ).

namespace F4MP_UI
{
    static char g_serverAddr[64] = "127.0.0.1";
    static int  g_serverPort = 7779;
    static bool g_connected = false;  // stub hasta el Hito 2 (red)

    // El framework llama a esta funcion cada frame para dibujar nuestra pagina.
    static void __stdcall RenderMenu()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            const auto pos = player->GetPosition();
            ImGuiMCP::Text("Tu posicion:  x=%.0f  y=%.0f  z=%.0f", pos.x, pos.y, pos.z);
        } else {
            ImGuiMCP::Text("Jugador no disponible");
        }

        ImGuiMCP::Separator();

        ImGuiMCP::InputText("Servidor", g_serverAddr, sizeof(g_serverAddr));
        ImGuiMCP::InputInt("Puerto", &g_serverPort);

        if (g_connected) {
            ImGuiMCP::Text("Estado: CONECTADO");
            if (ImGuiMCP::Button("Desconectar")) {
                g_connected = false;
                REX::INFO("[F4MP] (stub) desconectar");
            }
        } else {
            ImGuiMCP::Text("Estado: desconectado");
            if (ImGuiMCP::Button("Conectar")) {
                REX::INFO("[F4MP] (stub) conectar a {}:{}", g_serverAddr, g_serverPort);
                g_connected = true;  // stub: en el Hito 2 esto conectara de verdad
            }
        }
    }

    static void Register()
    {
        if (!F4SEMenuFramework::IsInstalled()) {
            REX::INFO("[F4MP] F4SE Menu Framework NO instalado: el menu no estara disponible");
            return;
        }
        F4SEMenuFramework::SetSection("F4MP");
        F4SEMenuFramework::AddSectionItem("Menu", RenderMenu);
        REX::INFO("[F4MP] menu registrado en F4SE Menu Framework");
    }
}

static void OnF4SEMessage(F4SE::MessagingInterface::Message* a_msg)
{
    if (!a_msg) return;
    // Registrar en kPostLoad/kPostPostLoad: asi el DLL del framework ya esta
    // cargado sin depender del orden de plugins.
    if (a_msg->type == F4SE::MessagingInterface::kPostLoad ||
        a_msg->type == F4SE::MessagingInterface::kPostPostLoad) {
        F4MP_UI::Register();
    }
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
    F4SE::Init(a_f4se);
    REX::INFO("[F4MP] Plugin cargado (CommonLibF4)");

    F4SE::GetMessagingInterface()->RegisterListener(OnF4SEMessage);
    return true;
}
