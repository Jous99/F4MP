#include "D3D11Hook.h"
#include "f4mp/Logger.h"

#include <detours/detours.h>
#include <vector>

namespace f4mp {

D3D11Hook& D3D11Hook::GetInstance() {
    static D3D11Hook instance;
    return instance;
}

bool D3D11Hook::Initialize() {
    if (m_initialized) {
        return true;
    }

    Logger::Info("Initializing D3D11 hook");

    if (!CreateDeviceAndSwapChain()) {
        Logger::Error("Failed to create D3D11 device for hooking");
        return false;
    }

    if (!HookPresent()) {
        Logger::Error("Failed to hook Present");
        return false;
    }

    m_initialized = true;
    Logger::Info("D3D11 hook initialized successfully");
    return true;
}

void D3D11Hook::Shutdown() {
    if (m_originalPresent) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&m_originalPresent, PresentHook);
        DetourTransactionCommit();
        m_originalPresent = nullptr;
    }

    if (m_context) m_context->Release();
    if (m_device) m_device->Release();
    if (m_swapChain) m_swapChain->Release();

    m_initialized = false;
    Logger::Info("D3D11 hook shutdown");
}

void D3D11Hook::SetRenderCallback(std::function<void(ID3D11DeviceContext*, ID3D11RenderTargetView*)> callback) {
    m_renderCallback = std::move(callback);
}

bool D3D11Hook::CreateDeviceAndSwapChain() {
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = GetForegroundWindow();
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &scd,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_context
    );

    if (FAILED(hr)) {
        Logger::Error("D3D11CreateDeviceAndSwapChain failed: 0x%08X", hr);
        return false;
    }

    Logger::Info("D3D11 device created with feature level %d", featureLevel);
    return true;
}

bool D3D11Hook::HookPresent() {
    void** vtable = *reinterpret_cast<void***>(m_swapChain);
    m_originalPresent = vtable[8];

    Logger::Info("Hooking IDXGISwapChain::Present at 0x%p", m_originalPresent);

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&m_originalPresent, PresentHook);
    LONG result = DetourTransactionCommit();

    if (result != NO_ERROR) {
        Logger::Error("DetourTransactionCommit failed: %ld", result);
        return false;
    }

    return true;
}

HRESULT __stdcall D3D11Hook::PresentHook(IDXGISwapChain* pSwapChain, UINT syncInterval, UINT flags) {
    auto& hook = GetInstance();

    if (hook.m_renderCallback) {
        ID3D11Texture2D* backBuffer = nullptr;
        pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

        if (backBuffer) {
            ID3D11RenderTargetView* rtv = nullptr;
            hook.m_device->CreateRenderTargetView(backBuffer, nullptr, &rtv);

            if (rtv) {
                hook.m_renderCallback(hook.m_context, rtv);
                rtv->Release();
            }

            backBuffer->Release();
        }
    }

    using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    return reinterpret_cast<PresentFn>(hook.m_originalPresent)(pSwapChain, syncInterval, flags);
}

}
