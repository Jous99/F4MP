#include "InputHook.h"
#include "f4mp/Logger.h"

namespace f4mp {

InputHook& InputHook::GetInstance() {
    static InputHook instance;
    return instance;
}

bool InputHook::Initialize(HWND hwnd) {
    if (m_initialized) {
        return true;
    }

    m_hwnd = hwnd;
    m_originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcHook)));

    if (!m_originalWndProc) {
        Logger::Error("Failed to subclass window procedure");
        return false;
    }

    m_initialized = true;
    Logger::Info("Input hook initialized");
    return true;
}

void InputHook::Shutdown() {
    if (m_originalWndProc && m_hwnd) {
        SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
        m_originalWndProc = nullptr;
    }
    m_initialized = false;
    Logger::Info("Input hook shutdown");
}

void InputHook::ToggleMenu() {
    m_menuVisible = !m_menuVisible;
    Logger::Info("Menu %s", m_menuVisible ? "shown" : "hidden");
}

LRESULT __stdcall InputHook::WndProcHook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto& hook = GetInstance();

    if (msg == WM_KEYDOWN && wParam == VK_DELETE) {
        hook.ToggleMenu();
        return 0;
    }

    if (hook.m_menuVisible) {
        switch (msg) {
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEMOVE:
            case WM_MOUSEWHEEL:
                return 0;
        }
    }

    return CallWindowProc(hook.m_originalWndProc, hwnd, msg, wParam, lParam);
}

}
