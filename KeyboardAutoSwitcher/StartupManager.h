#pragma once
#include <winrt/Windows.ApplicationModel.h>

enum class StartupState {
    Enabled,
    Disabled,
    DisabledByUser,
    DisabledByPolicy,
    NotConfigured,
    Error,
    NotSupported
};

class StartupManager {
public:
    StartupManager();

    StartupState GetState();
    bool RequestEnable();   // òîëüêî åñëè Disabled
    void OpenStartupSettings(); // ms-settings

private:
    winrt::Windows::ApplicationModel::StartupTask m_task{ nullptr };
};
