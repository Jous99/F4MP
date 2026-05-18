#pragma once

#include <Windows.h>

namespace f4mp {

class InputHook {
public:
    static InputHook& GetInstance();

    bool Initialize(HWND hwnd);
    void Shutdown();

    bool IsMenuVisible() const { return m_menuVisible; }
    void ToggleMenu();

private:
    InputHook() = default;

    static LRESULT __stdcall WndProcHook(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = nullptr;
    WNDPROC m_originalWndProc = nullptr;
    bool m_menuVisible = false;
    bool m_initialized = false;
};

}
