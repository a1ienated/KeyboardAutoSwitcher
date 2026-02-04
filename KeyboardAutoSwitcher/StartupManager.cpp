#include "stdafx.h"
#include "StartupManager.h"
#include <winrt/Windows.Foundation.h>
#include <shellapi.h>

using namespace winrt;
using namespace Windows::ApplicationModel;

StartupManager::StartupManager()
{
    try {
        init_apartment(apartment_type::single_threaded);
        m_task = StartupTask::GetAsync(L"KeyboardAutoSwitcherStartup").get();
    }
    catch (...) {
        m_task = nullptr;
    }
}

StartupState StartupManager::GetState()
{
    if (!m_task)
        return StartupState::NotConfigured;

    switch (m_task.State()) {
    case StartupTaskState::Enabled:
        return StartupState::Enabled;

    case StartupTaskState::Disabled:
        return StartupState::Disabled;

    case StartupTaskState::DisabledByUser:
        return StartupState::DisabledByUser;

    case StartupTaskState::DisabledByPolicy:
        return StartupState::DisabledByPolicy;

    default:
        return StartupState::Error;
    }
}

bool StartupManager::RequestEnable()
{
    if (!m_task)
        return false;

    if (m_task.State() != StartupTaskState::Disabled)
        return false;

    try {
        auto result = m_task.RequestEnableAsync().get();
        return result == StartupTaskState::Enabled;
    }
    catch (...) {
        return false;
    }
}

void StartupManager::OpenStartupSettings()
{
    ShellExecute(
        nullptr,
        L"open",
        L"ms-settings:startupapps",
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );
}
