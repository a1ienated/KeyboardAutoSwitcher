#include "stdafx.h"
#include "KeyboardHook.h"

#include <utility>

KeyboardHook* KeyboardHook::s_instance = nullptr;

KeyboardHook::~KeyboardHook()
{
    Uninstall();
}

void KeyboardHook::SetCallback(Callback cb)
{
    m_callback = std::move(cb);
}

bool KeyboardHook::Install()
{
    if (m_hook) return true;

    // this implementation supports only one active instance
    if (s_instance && s_instance != this) return false;

    s_instance = this;

    const HMODULE hModule = GetModuleHandleW(nullptr);
    m_hook = SetWindowsHookExW(WH_KEYBOARD_LL, &KeyboardHook::HookProc, hModule, 0);

    if (!m_hook)
    {
        s_instance = nullptr;
        return false;
    }

    return true;
}

void KeyboardHook::Uninstall()
{
    if (m_hook)
    {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }

    if (s_instance == this)
        s_instance = nullptr;
}

static KeyEvent MakeEvent(WPARAM wParam, const KBDLLHOOKSTRUCT& native)
{
    KeyEvent e;
    e.vkCode = static_cast<std::uint32_t>(native.vkCode);
    e.scanCode = static_cast<std::uint32_t>(native.scanCode);
    e.flags = static_cast<std::uint32_t>(native.flags);
    e.time = static_cast<std::uint32_t>(native.time);
    e.extraInfo = static_cast<std::uintptr_t>(native.dwExtraInfo);

    e.isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    e.isShiftDown= (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    e.isCtrlDown = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

    e.isKeyUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
    e.isSysKey = (wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP);

    e.altDown = ((native.flags & LLKHF_ALTDOWN) != 0);
    e.injected = ((native.flags & LLKHF_INJECTED) != 0);

    return e;
}

LRESULT CALLBACK KeyboardHook::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return CallNextHookEx(nullptr, nCode, wParam, lParam);

    KeyboardHook* self = s_instance;
    if (!self)
        return CallNextHookEx(nullptr, nCode, wParam, lParam);

    if (nCode == HC_ACTION)
    {
        const auto* native = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        if (native && self->m_callback)
        {
            const KeyEvent e = MakeEvent(wParam, *native);
            const bool block = self->m_callback(e);
            if (block)
                return 1;
        }
    }

    return CallNextHookEx(self->m_hook, nCode, wParam, lParam);
}
