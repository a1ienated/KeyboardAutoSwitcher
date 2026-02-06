#include "stdafx.h"
#include "StartupManager.h"
#include <winrt/Windows.Foundation.h>
#include <shellapi.h>
#include <appmodel.h>

#include <thread>
#include <winrt/Windows.ApplicationModel.h>


using namespace winrt;
using namespace Windows::ApplicationModel;


static constexpr wchar_t kTaskId[] = L"KeyboardAutoSwitcherStartup";

static bool IsPackagedProcess()
{
    UINT32 len = 0;
    return GetCurrentPackageFullName(&len, nullptr) != APPMODEL_ERROR_NO_PACKAGE;
}

void StartupManager::EnsureInitializedAsync(HWND hWnd)
{
    KAS_DLOGF(L"[StartupManager] EnsureInitializedAsync start");
    // Set the status to Loading; the UI can display "Loading..."
    m_state.store(StartupState::Loading);

    std::thread([this, hWnd]()
        {
            KAS_DLOGF(L"[StartupManager] worker thread started");

            if (!IsPackagedProcess()) {
                KAS_DLOGF(L"[StartupManager] NOT PACKAGED");
                m_state.store(StartupState::NotConfigured);
                if (IsWindow(hWnd)) PostMessageW(hWnd, WM_APP_STARTUP_STATE_READY, 0, 0);
                return;
            }

            try {
                // IMPORTANT: This is not the UI thread, .get() is acceptable.
                winrt::init_apartment(winrt::apartment_type::multi_threaded);
                auto task = StartupTask::GetAsync(kTaskId).get();
                KAS_DLOGF(L"[StartupManager] task.State=%d", (int)task.State());

                switch (task.State()) {
                case StartupTaskState::Enabled:
                    m_state.store(StartupState::Enabled); break;
                case StartupTaskState::Disabled:
                    m_state.store(StartupState::Disabled); break;
                case StartupTaskState::DisabledByUser:
                    m_state.store(StartupState::DisabledByUser); break;
                case StartupTaskState::DisabledByPolicy:
                    m_state.store(StartupState::DisabledByPolicy); break;
                default:
                    m_state.store(StartupState::NotConfigured); break;
                }
            }
            catch (...) {
                KAS_DLOGF(L"[StartupManager] exception");
                m_state.store(StartupState::NotConfigured);
            }

            if (IsWindow(hWnd))
                PostMessageW(hWnd, WM_APP_STARTUP_STATE_READY, 0, 0);
        }).detach();
}

bool StartupManager::RequestEnable()
{
    try
    {
        // (optional) quick guard
        if (!IsPackagedProcess()) {
            KAS_DLOG(L"[Startup] RequestEnable: not packaged");
            return false;
        }
        // Initialize the WinRT apartment for the current thread.
        // If the thread is already STA, simply continue (it's important that the apartment is initialized, not the type).
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
        }
        catch (winrt::hresult_error const& e) {
            // 0x80010106 = RPC_E_CHANGED_MODE (stream has already been initialized with a different type)
            if ((uint32_t)e.code().value != 0x80010106) throw;
        }

        auto task = winrt::Windows::ApplicationModel::StartupTask::GetAsync(kTaskId).get();
        auto result = task.RequestEnableAsync().get();

        KAS_DLOGF(L"[Startup] RequestEnableAsync result=%d", (int)result);
        return (result == winrt::Windows::ApplicationModel::StartupTaskState::Enabled);
    }
    catch (winrt::hresult_error const& e)
    {
        KAS_DLOGF(L"[Startup] EX: 0x%08X %s",
            (uint32_t)e.code().value,
            e.message().c_str());
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
