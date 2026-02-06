#pragma once

#include <Windows.h>
#include <cstdint>
#include <functional>

struct KeyEvent
{
    std::uint32_t vkCode = 0;
    std::uint32_t scanCode = 0;
    std::uint32_t flags = 0;
    std::uint32_t time = 0;
    std::uintptr_t extraInfo = 0;

    bool isKeyDown = false;
    bool isKeyUp = false;
    bool isSysKey = false;
    bool altDown = false;
    bool injected = false;
};

class KeyboardHook
{
public:
    // return true => block key event (do not pass to Windows)
    using Callback = std::function<bool(const KeyEvent&)>;

    KeyboardHook() = default;
    KeyboardHook(const KeyboardHook&) = delete;
    KeyboardHook& operator=(const KeyboardHook&) = delete;

    ~KeyboardHook();

    void SetCallback(Callback cb);

    bool Install();
    void Uninstall();

    bool IsInstalled() const noexcept { return m_hook != nullptr; }

private:
    static LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    static KeyboardHook* s_instance;

    HHOOK m_hook = nullptr;
    Callback m_callback;
};
