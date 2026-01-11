#ifndef H_DIRECT3D11
#define H_DIRECT3D11

#include <d3d11.h>
#include <vector>

namespace hDirect3D11 {
    class Direct3D11 {
    public:
        std::vector<uintptr_t> vtable_methods;

        Direct3D11() {
            ID3D11Device* pDevice = nullptr;
            ID3D11DeviceContext* pContext = nullptr;
            IDXGISwapChain* pSwapChain = nullptr;

            D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
            DXGI_SWAP_CHAIN_DESC scd = {};
            scd.BufferCount = 1;
            scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            scd.OutputWindow = GetForegroundWindow();
            scd.SampleDesc.Count = 1;
            scd.Windowed = TRUE;
            scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1, D3D11_SDK_VERSION, &scd, &pSwapChain, &pDevice, nullptr, &pContext))) {
                uintptr_t* vtable = *(uintptr_t**)pSwapChain;
                for (int i = 0; i < 150; i++) {
                    vtable_methods.push_back(vtable[i]);
                }
                pSwapChain->Release();
                pDevice->Release();
                pContext->Release();
            }
        }

        uintptr_t* vtable() { return vtable_methods.data(); }
    };
}

namespace hDXGI {
    enum Index { Present = 8 };
}
#endif