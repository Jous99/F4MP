#include "DirectXHook.h"
#include <mutex>

// Definición de variables globales
HWND g_windowHandle = NULL;
ID3D11Device* g_d3d11Device = NULL;
ID3D11DeviceContext* g_d3d11Context = NULL;
IDXGISwapChain* g_d3d11SwapChain = NULL;
ID3D11RenderTargetView* g_mainRenderTargetView = NULL;
bool g_ShowMenu = false;
bool g_Initialized = false;

namespace Hooks {
    namespace DirectX {

        void Init() {
            try {
                auto d3d11 = std::make_unique<hDirect3D11::Direct3D11>();
                uintptr_t* vtable = d3d11->vtable();

                spdlog::info("[F4MP] Hooking DXGI Present...");

                swapChainPresent11Hook.apply(vtable[hDXGI::Present], [](IDXGISwapChain* chain, UINT SyncInterval, UINT Flags) -> HRESULT {
                    static std::once_flag flag;
                    std::call_once(flag, [&chain]() {
                        g_d3d11SwapChain = chain;
                        if (SUCCEEDED(chain->GetDevice(__uuidof(ID3D11Device), (void**)&g_d3d11Device))) {
                            Pre_Render(chain);
                            g_Initialized = true;
                        }
                    });

                    Render();
                    return swapChainPresent11Hook.call_orig(chain, SyncInterval, Flags);
                });
            }
            catch (const std::exception& e) {
                spdlog::error("[F4MP] DX11 Hook Error: {}", e.what());
            }
        }

        void Pre_Render(IDXGISwapChain* swapChain) {
            swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_d3d11Device);
            g_d3d11Device->GetImmediateContext(&g_d3d11Context);

            DXGI_SWAP_CHAIN_DESC sd;
            swapChain->GetDesc(&sd);
            g_windowHandle = sd.OutputWindow;

            // Hook al WndProc para el ratón
            OriginalWndProcHandler = (WNDPROC)SetWindowLongPtr(g_windowHandle, GWLP_WNDPROC, (LONG_PTR)hWndProc);

            ImGui::CreateContext();
            ImGui_ImplWin32_Init(g_windowHandle);
            ImGui_ImplDX11_Init(g_d3d11Device, g_d3d11Context);

            ID3D11Texture2D* pBackBuffer;
            swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            g_d3d11Device->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
            pBackBuffer->Release();
        }

        void Render() {
            if (!g_Initialized) return;

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (g_ShowMenu) {
                ImGui::Begin("F4MP Next-Gen Console", &g_ShowMenu);
                ImGui::Text("F4MP Experimental Multiplayer Mod");
                ImGui::Separator();
                if (ImGui::Button("Unload Mod")) { /* Lógica de salida */ }
                ImGui::End();
            }

            ImGui::Render();
            g_d3d11Context->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
    }
}