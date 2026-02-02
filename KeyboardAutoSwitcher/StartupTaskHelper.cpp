#include "stdafx.h"
#include <windows.h>
#include <taskschd.h>
#include <comdef.h>
#include <atlbase.h>
#include <string>

// Task Scheduler
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")

#include "Log/logger.h"
#include "StartupTaskHelper.h"

// ------------------------------------------------------------
// const
// ------------------------------------------------------------
static constexpr const wchar_t* TASK_FOLDER = L"\\KeyboardAutoSwitcher";
static constexpr const wchar_t* TASK_NAME = L"KAS";

// ------------------------------------------------------------
// Helper: path to exe and work dir
// ------------------------------------------------------------
static std::wstring GetExePath()
{
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    return exePath;
}

static std::wstring GetExeDir(const std::wstring& exePath)
{
    auto pos = exePath.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return L"";
    return exePath.substr(0, pos);
}

// ------------------------------------------------------------
// Helper: curerent username in SAM-format "PC\\User"
// ------------------------------------------------------------
static std::wstring GetCurrentUserSam()
{
    wchar_t user[256]{};
    DWORD userLen = (DWORD)std::size(user);
    if (!GetUserNameW(user, &userLen))
        return L"";

    wchar_t comp[256]{};
    DWORD compLen = (DWORD)std::size(comp);
    if (!GetComputerNameW(comp, &compLen))
        return user; // fallback

    std::wstring sam = comp;
    sam += L"\\";
    sam += user;
    return sam;
}

// ------------------------------------------------------------
// Helper: open root folder
// ------------------------------------------------------------
static HRESULT GetRootFolder(ITaskService* service, ITaskFolder** root)
{
    if (!service || !root) return E_INVALIDARG;
    *root = nullptr;
    return service->GetFolder(_bstr_t(L"\\"), root);
}

// ------------------------------------------------------------
// Helper: get/make app folder
// ------------------------------------------------------------
static HRESULT GetOrCreateAppFolder(ITaskFolder* root, ITaskFolder** appFolder)
{
    if (!root || !appFolder) return E_INVALIDARG;
    *appFolder = nullptr;

    HRESULT hr = root->GetFolder(_bstr_t(TASK_FOLDER), appFolder);
    if (SUCCEEDED(hr))
        return hr;

    // Create
    CComPtr<ITaskFolder> created;
    hr = root->CreateFolder(_bstr_t(TASK_FOLDER), _variant_t(), &created);
    if (SUCCEEDED(hr))
    {
        *appFolder = created.Detach();
    }
    return hr;
}

// ------------------------------------------------------------
// EnableStartup
// ------------------------------------------------------------
bool startup::EnableStartup()
{
    logger::Info(L"EnableStartup: begin");

    const std::wstring userSam = GetCurrentUserSam();
    if (userSam.empty())
    {
        logger::Error(L"EnableStartup: cannot resolve current user");
        return false;
    }
    logger::Info(L"EnableStartup: user=%s", userSam.c_str());

    const std::wstring exePath = GetExePath();
    const std::wstring exeDir = GetExeDir(exePath);
    logger::Info(L"EnableStartup: exe=%s", exePath.c_str());
    logger::Info(L"EnableStartup: dir=%s", exeDir.c_str());

    // 1) Connect to Scheduler
    CComPtr<ITaskService> service;
    HRESULT hr = service.CoCreateInstance(CLSID_TaskScheduler);
    if (FAILED(hr)) { logger::Hr(L"CoCreateInstance(CLSID_TaskScheduler)", hr); return false; }

    hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) { logger::Hr(L"ITaskService::Connect", hr); return false; }

    // 2) Root folder
    CComPtr<ITaskFolder> root;
    hr = GetRootFolder(service, &root);
    if (FAILED(hr)) { logger::Hr(L"GetRootFolder", hr); return false; }

    // 3) Get/make app folder
    CComPtr<ITaskFolder> appFolder;
    hr = GetOrCreateAppFolder(root, &appFolder);
    if (FAILED(hr)) { logger::Hr(L"GetOrCreateAppFolder", hr); return false; }

    // 4) Create definition
    CComPtr<ITaskDefinition> task;
    hr = service->NewTask(0, &task);
    if (FAILED(hr)) { logger::Hr(L"ITaskService::NewTask", hr); return false; }

    // 4.1) Registration info
    {
        CComPtr<IRegistrationInfo> regInfo;
        task->get_RegistrationInfo(&regInfo);
        if (regInfo)
        {
            regInfo->put_Author(_bstr_t(L"Axion Technology"));
            regInfo->put_Description(_bstr_t(L"Start KeyboardAutoSwitcher on user logon"));
        }
    }

    // 4.2) Principal (so that it would definitely be "only when the user is logged in")
    {
        CComPtr<IPrincipal> principal;
        task->get_Principal(&principal);
        if (principal)
        {
            principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
            principal->put_RunLevel(TASK_RUNLEVEL_LUA); // без "highest"
            // principal->put_UserId(_bstr_t(userSam.c_str())); // it is possible, but not necessarily
        }
    }

    // 4.3) Settings
    {
        CComPtr<ITaskSettings> settings;
        task->get_Settings(&settings);
        if (settings)
        {
            settings->put_StartWhenAvailable(VARIANT_TRUE);
            settings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
            settings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
            settings->put_ExecutionTimeLimit(_bstr_t(L"PT0S")); // без лимита
            settings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW);
        }
    }

    // 5) Triggers: Logon, BIND to current user (Important!)
    {
        CComPtr<ITriggerCollection> triggers;
        hr = task->get_Triggers(&triggers);
        if (FAILED(hr)) { logger::Hr(L"get_Triggers", hr); return false; }

        CComPtr<ITrigger> trigger;
        hr = triggers->Create(TASK_TRIGGER_LOGON, &trigger);
        if (FAILED(hr)) { logger::Hr(L"triggers->Create(TASK_TRIGGER_LOGON)", hr); return false; }

        CComPtr<ILogonTrigger> logonTrigger;
        hr = trigger->QueryInterface(IID_PPV_ARGS(&logonTrigger));
        if (FAILED(hr)) { logger::Hr(L"QueryInterface(ILogonTrigger)", hr); return false; }

        hr = logonTrigger->put_UserId(_bstr_t(userSam.c_str()));
        if (FAILED(hr)) { logger::Hr(L"ILogonTrigger::put_UserId", hr); return false; }
    }

    // 6) Actions: Exec + WorkingDirectory
    {
        CComPtr<IActionCollection> actions;
        hr = task->get_Actions(&actions);
        if (FAILED(hr)) { logger::Hr(L"get_Actions", hr); return false; }

        CComPtr<IAction> action;
        hr = actions->Create(TASK_ACTION_EXEC, &action);
        if (FAILED(hr)) { logger::Hr(L"actions->Create(TASK_ACTION_EXEC)", hr); return false; }

        CComPtr<IExecAction> exec;
        hr = action->QueryInterface(IID_PPV_ARGS(&exec));
        if (FAILED(hr)) { logger::Hr(L"QueryInterface(IExecAction)", hr); return false; }

        hr = exec->put_Path(_bstr_t(exePath.c_str()));
        if (FAILED(hr)) { logger::Hr(L"IExecAction::put_Path", hr); return false; }

        if (!exeDir.empty())
        {
            hr = exec->put_WorkingDirectory(_bstr_t(exeDir.c_str()));
            if (FAILED(hr)) { logger::Hr(L"IExecAction::put_WorkingDirectory", hr); return false; }
        }
    }

    // 7) Register (important: security descriptor = VT_EMPTY)
    {
        CComPtr<IRegisteredTask> registered;

        hr = appFolder->RegisterTaskDefinition(
            _bstr_t(TASK_NAME),
            task,
            TASK_CREATE_OR_UPDATE,
            _variant_t(),                    // user: default
            _variant_t(),                    // password: empty
            TASK_LOGON_INTERACTIVE_TOKEN,
            _variant_t(),                    // ✅ security descriptor: VT_EMPTY
            &registered);

        if (FAILED(hr)) { logger::Hr(L"RegisterTaskDefinition", hr); return false; }
    }

    logger::Info(L"EnableStartup: OK");
    return true;
}

// ------------------------------------------------------------
// DisableStartup
// ------------------------------------------------------------
bool startup::DisableStartup()
{
    logger::Info(L"DisableStartup: begin");

    CComPtr<ITaskService> service;
    HRESULT hr = service.CoCreateInstance(CLSID_TaskScheduler);
    if (FAILED(hr)) { logger::Hr(L"CoCreateInstance(CLSID_TaskScheduler)", hr); return false; }

    hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) { logger::Hr(L"ITaskService::Connect", hr); return false; }

    CComPtr<ITaskFolder> root;
    hr = GetRootFolder(service, &root);
    if (FAILED(hr)) { logger::Hr(L"GetRootFolder", hr); return false; }

    // 1) Try to remove from our folder
    CComPtr<ITaskFolder> appFolder;
    hr = root->GetFolder(_bstr_t(TASK_FOLDER), &appFolder);

    if (SUCCEEDED(hr) && appFolder)
    {
        hr = appFolder->DeleteTask(_bstr_t(TASK_NAME), 0);
        if (SUCCEEDED(hr))
        {
            logger::Info(L"DisableStartup: task deleted from %s", TASK_FOLDER);
            return true;
        }

        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            logger::Info(L"DisableStartup: task not found in %s", TASK_FOLDER);
            return true;
        }

        logger::Hr(L"DeleteTask in app folder failed", hr);
        return false;
    }

    // 2) Fallback: Suddenly, something old is lying at the root somewhere.
    hr = root->DeleteTask(_bstr_t(TASK_NAME), 0);
    if (SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        logger::Info(L"DisableStartup: fallback delete in root OK/not found");
        return true;
    }

    logger::Hr(L"DisableStartup: fallback root delete failed", hr);
    return false;
}

bool startup::IsStartupEnabled()
{
    logger::Info(L"IsStartupEnabled: begin");

    CComPtr<ITaskService> service;
    HRESULT hr = service.CoCreateInstance(CLSID_TaskScheduler);
    if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: CoCreateInstance", hr); return false; }

    hr = service->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: Connect", hr); return false; }

    CComPtr<ITaskFolder> root;
    hr = GetRootFolder(service, &root);
    if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: GetRootFolder", hr); return false; }

    // 1) First, let's try the "correct" location: \KeyboardAutoSwitcher\KAS
    CComPtr<ITaskFolder> appFolder;
    hr = root->GetFolder(_bstr_t(TASK_FOLDER), &appFolder);

    if (SUCCEEDED(hr) && appFolder)
    {
        CComPtr<IRegisteredTask> task;
        hr = appFolder->GetTask(_bstr_t(TASK_NAME), &task);

        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            logger::Info(L"IsStartupEnabled: task not found in %s", TASK_FOLDER);
            return false;
        }
        if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: GetTask (app folder)", hr); return false; }

        VARIANT_BOOL enabled = VARIANT_FALSE;
        hr = task->get_Enabled(&enabled);
        if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: get_Enabled", hr); return false; }

        logger::Info(L"IsStartupEnabled: found in %s, enabled=%d", TASK_FOLDER, enabled == VARIANT_TRUE);
        return enabled == VARIANT_TRUE;
    }

    // If the folder doesn't exist, this is NOT an error; it simply means that automatic startup is not configured.
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        logger::Info(L"IsStartupEnabled: folder %s not found", TASK_FOLDER);
        return false;
    }

    // Other errors when opening the folder - log them.
    if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: GetFolder(app)", hr); return false; }

    // 2) Fallback: perhaps the old task is located in the root directory "\KAS"
    {
        CComPtr<IRegisteredTask> task;
        hr = root->GetTask(_bstr_t(TASK_NAME), &task);

        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            logger::Info(L"IsStartupEnabled: task not found in root");
            return false;
        }
        if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: GetTask(root)", hr); return false; }

        VARIANT_BOOL enabled = VARIANT_FALSE;
        hr = task->get_Enabled(&enabled);
        if (FAILED(hr)) { logger::Hr(L"IsStartupEnabled: get_Enabled(root)", hr); return false; }

        logger::Info(L"IsStartupEnabled: found in root, enabled=%d", enabled == VARIANT_TRUE);
        return enabled == VARIANT_TRUE;
    }
}


