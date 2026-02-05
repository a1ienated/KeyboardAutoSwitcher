#pragma once
#include <atomic>

enum class StartupState {
    NotConfigured,
    Enabled,
    Disabled,
    DisabledByUser,
    DisabledByPolicy,
    Loading
};

constexpr UINT WM_APP_STARTUP_STATE_READY = WM_APP + 201;

class StartupManager {
public:
    StartupManager() = default;

    void EnsureInitializedAsync(HWND hWnd);
    StartupState GetCachedState() const { return m_state.load(); }

    bool RequestEnable();          // worker-safe)
    void OpenStartupSettings();

private:
    std::atomic<StartupState> m_state{ StartupState::NotConfigured };
    //static std::atomic_bool g_inFlight{ false };
};
