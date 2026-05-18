#pragma once

#include <d3d11.h>
#include <functional>

namespace f4mp {

class D3D11Hook {
public:
    static D3D11Hook& GetInstance();

    bool Initialize();
    void Shutdown();

    void SetRenderCallback(std::function<void(ID3D11DeviceContext*, ID3D11RenderTargetView*)> callback);

private:
    D3D11Hook() = default;

    bool CreateDeviceAndSwapChain();
    bool HookPresent();

    static HRESULT __stdcall PresentHook(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags);

    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;

    void* m_originalPresent = nullptr;

    std::function<void(ID3D11DeviceContext*, ID3D11RenderTargetView*)> m_renderCallback;

    bool m_initialized = false;
};

}
