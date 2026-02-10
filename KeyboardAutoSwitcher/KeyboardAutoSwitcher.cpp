#include "stdafx.h"
#include "KeyboardAutoSwitcher.h"
#include "KeyboardLayoutManager.h"
#include "StartupManager.h"
#include "KeyboardHook.h"
#include "StateManager.h"
#include <tchar.h>
#include <xstring>
#include <strsafe.h>

#include <atomic>
#include <thread>
#include <winrt/Windows.ApplicationModel.h>

typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>>String;

#define MAX_LOADSTRING 100
#define MAX_LINE 3
#define MAX_CHAR 100

#define STRLEN(x) (sizeof(x)/sizeof(TCHAR) - 1)

// global var
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hCheckbx;

static constexpr UINT kLayoutIdleTimeoutMs = 20000;
static constexpr UINT kUI_IdleTimeoutMs = 50;

wchar_t info[MAX_LINE][MAX_CHAR];
NOTIFYICONDATA nid = {};

static KeyboardHook g_keyboardHook;

static std::atomic_bool g_startupEnableInFlight{ false };
static StartupManager g_startupManager;

struct GlobalState{
	bool m_disableChecked{ false };
	bool m_isLayoutDefault{false};
}g_state;

static Timer* g_tLayout = nullptr;
static Timer* g_tUI = nullptr;

// Forward declarations of functions included in this code module:
ATOM RegMyWindowClass(HINSTANCE hInst, LPCTSTR lpzClassName);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static bool HandleKeyEvent(const KeyEvent& e);
static bool HandleTimerEvent(const StateEvent& e);
static void RefreshWindow(HWND hWnd);

// main window
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Prevent from two copies of app from running at the same time
	HANDLE mutex = CreateMutex(NULL, FALSE, L"KeyboardAutoSwitcher");
	if (!mutex || WaitForSingleObject(mutex, 0) == WAIT_TIMEOUT) {
		KAS_DLOGF(L"KeyboardAutoSwitcher is already running.");
		return 1;
	}

//#ifdef _DEBUG
	winrt::init_apartment(winrt::apartment_type::single_threaded);
//#endif

	// Store instance handle in our global variable.
	hInst = hInstance;

	GetKeyboardLayouts();

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_KEYBOARDAUTOSWITCHER, szWindowClass, MAX_LOADSTRING);

	// register class
	if (!RegMyWindowClass(hInstance, szWindowClass))
		return 1;

	// Create the main program window.
	HWND hWnd = CreateWindowW(szWindowClass,
		szTitle,
		WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT,
		0,
		400,
		200,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	if (!hWnd) return -1;

	StateManager::GetInstance().SetCallback(HandleTimerEvent);
	StateManager::GetInstance().RegisterTimer(std::make_unique<Timer>(hWnd, (UINT_PTR)1, TimerType::LayoutSwitch));
	StateManager::GetInstance().RegisterTimer(std::make_unique<Timer>(hWnd, (UINT_PTR)2, TimerType::UI));
	g_tLayout = StateManager::GetInstance().onTimer(TimerType::LayoutSwitch);
	g_tUI = StateManager::GetInstance().onTimer(TimerType::UI);

	// Set hook to capture keyboard events
	g_keyboardHook.SetCallback(HandleKeyEvent);
	if (!g_keyboardHook.Install()) {
		KAS_DLOGF(L"[Hook] Failed to install WH_KEYBOARD_LL hook. err=%lu", GetLastError());
	}

	// Display the main program window.
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	// Loads the specified accelerator table (hot key)
	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYBOARDAUTOSWITCHER));

	// Main message loop
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

ATOM RegMyWindowClass(HINSTANCE hInstance, LPCTSTR lpzClassName)
{
	//  Registers the window class.
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW | WS_OVERLAPPED;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE((LPCTSTR)IDI_KEYBOARDAUTOSWITCHER));
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE((LPCTSTR)IDI_SMALL));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_KEYBOARDAUTOSWITCHER);
	wcex.lpszClassName = lpzClassName;
	return RegisterClassExW(&wcex);
}

static void OnLButtonDoubleClick(HWND hWnd)
{
//	if (IsWindowVisible(hWnd) == TRUE)
	{
		//ShowWindow(hWnd, SW_HIDE);
		ShowWindow(hWnd, SW_RESTORE);
		//ShowWindow(hWnd, SW_SHOW);
	}
}

// show pop-up menu
static void OnContextMenu(HWND hWnd, int x, int y)
{
	HMENU hMenu = GetMenu(hWnd);
	HMENU hSubMenu = GetSubMenu(hMenu, 0);

	TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, hWnd, NULL);
}

static void OnDisable(HWND hWnd, int x, int y)
{
	HMENU hMenu = GetMenu(hWnd);
	HMENU hSubMenu = GetSubMenu(hMenu, 0);

	//CheckMenuItem(hSubMenu, IDM_DISABLE, MF_BYCOMMAND | MF_CHECKED);
	g_state.m_disableChecked = !g_state.m_disableChecked;
	MENUITEMINFO menuItem = { 0 };
	menuItem.cbSize = sizeof(MENUITEMINFO);
	menuItem.fMask = MIIM_STATE;
	menuItem.fState = g_state.m_disableChecked ? MF_CHECKED : MF_UNCHECKED;
	SetMenuItemInfo(hSubMenu, 1, TRUE, &menuItem);

//	DestroyMenu(hMenu);
}

// render main window
static void OnPaint(HWND hWnd)
{
//	TCHAR strCount[10], strTm[20] = _T("Timer: ");
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	//_tcscat(strTm, _itot(countTimerSec, strCount, 10));
	//TextOut(hDC, 20, 10, strTm, _tcsclen(strTm));

	TCHAR labelLayout[256];
	g_keyboardInfo.currentSlotLang = GetIndexLanguage();
	//labelLayout = _T("Language: ");
	//_tcscat(labelLayout, g_keyboardInfo.names[g_keyboardInfo.currentSlotLang]);

	UINT idx = g_keyboardInfo.currentSlotLang;
	if (idx >= g_keyboardInfo.count || idx >= MAX_LAYOUTS) idx = 0;
	const TCHAR* name = g_keyboardInfo.names[idx];
	if (!name || name[0] == 0) name = _T("(unknown)");
	StringCchPrintf(labelLayout, _countof(labelLayout), _T("Language: %s"), name);

	TextOut(hDC, 20, 30, labelLayout, (int)_tcslen(labelLayout));

	// get spacing between lines
	int i = 0, x = 20, y = 80, d = 0;
	TEXTMETRIC tm;
	GetTextMetrics(hDC, &tm);
	d = tm.tmHeight + tm.tmExternalLeading;

	for (; i < MAX_LINE; i++) {
		TextOut(hDC, x, y, info[i], wcslen(info[i]));
		y += d;
	}

	EndPaint(hWnd, &ps);
}

static void OnCreate(HWND hWnd)
{
	wcscpy_s(info[0], MAX_CHAR, L"Caps Lock - default layout");
	wcscpy_s(info[1], MAX_CHAR, L"Shift + Caps Lock - second layout");
	swprintf_s(info[2], MAX_CHAR, L"%u sec timer - on last input switches to default layout", (kLayoutIdleTimeoutMs / 1000));

	// add icon in system tray
	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uID = IDI_KEYBOARDAUTOSWITCHER;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
	nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_KEYBOARDAUTOSWITCHER));
	nid.uCallbackMessage = WM_SYSTEM_TRAY_ICON;
	nid.uVersion = NOTIFYICON_VERSION_4;
	wcscpy_s(nid.szTip, 128, L"KeyboardAutoSwitcher - portable");

	Shell_NotifyIcon(NIM_ADD, &nid);
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

static void OnDestroy(HWND hWnd)
{
	//StateManager::GetInstance().Shutdown();
	g_keyboardHook.Uninstall();
	g_startupEnableInFlight = false;
	EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), TRUE);

	// remove icon from system tray
	Shell_NotifyIcon(NIM_DELETE, &nid);
	PostQuitMessage(0);
}

static void OnClose(HWND hWnd)
{
	ShowWindow(hWnd, SW_HIDE);
}

// display about dialog box
static void OnFileAbout(HWND hWnd, HINSTANCE hInstance)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, DlgProc);
}

static void OnFileExit(HWND hWnd)
{
	DestroyWindow(hWnd);
}

static void RefreshWindow(HWND hWnd)
{
	// language display text
	InvalidateRect(hWnd, NULL, TRUE);
}

static void RefreshStartupUI(HWND hWnd)
{
	// during RequestEnableAsync — checkbox always disabled
	if (g_startupEnableInFlight.load()) {
		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), FALSE);
		// optional: SetWindowTextW(..., L"Enabling...");
		return;
	}

	const auto state = g_startupManager.GetCachedState();
	KAS_DLOGF(L"[StartupUI] state=%d", (int)state);

	switch (state)
	{
	case StartupState::Loading:
		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), FALSE);
		break;

	case StartupState::Enabled:
		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_CHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), TRUE);
		break;

	case StartupState::Disabled:
		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), TRUE);
		break;

	case StartupState::DisabledByUser:
		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), TRUE);   // click → to open Settings
		break;

	case StartupState::DisabledByPolicy:
		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), FALSE);  // policy = block
		break;

	case StartupState::NotConfigured:
	default:
		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), TRUE);
		break;
	}
}

static void HandleStartupCheckbox(HWND hWnd)
{
	KAS_DLOGF(L"[Checkbox] clicked()");
	const StartupState state = g_startupManager.GetCachedState();
	KAS_DLOGF(L"[StartupClick] state=%d", (int)state);

	switch (state)
	{
	case StartupState::Enabled:
	{
		int rc = MessageBoxW(
			hWnd,
			L"To disable startup, please use Windows Settings:\n"
			L"Settings → Apps → Startup\n\n"
			L"Open Settings now?",
			L"Disable startup",
			MB_OKCANCEL | MB_ICONINFORMATION
		);

		if (rc == IDOK)
			g_startupManager.OpenStartupSettings();

		break;
	}
	case StartupState::Disabled:
	{
		if (g_startupEnableInFlight.exchange(true)) return;

		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), FALSE);

		// SetWindowTextW(GetDlgItem(hWnd, IDM_CHECKBOX), L"Enabling...");
		// protection against race conditions
		std::thread([hWnd]() {
			KAS_DLOGF(L"[StartupWorker] RequestEnable()");

			if (!hWnd)
				return;
			winrt::init_apartment(winrt::apartment_type::multi_threaded);

			g_startupManager.RequestEnable();
			g_startupManager.EnsureInitializedAsync(hWnd); // refresh state
			}).detach();

		break;
	}
	case StartupState::DisabledByUser:
	{
		int rc = MessageBoxW(
			hWnd,
			L"Startup is turned off in Windows settings.\n"
			L"Please enable Keyboard Auto Switcher in:\n"
			L"Settings → Apps → Startup\n\nOpen Settings now?",
			L"Startup disabled",
			MB_OKCANCEL | MB_ICONINFORMATION
		);

		if (rc != IDOK)
			return;

		g_startupManager.OpenStartupSettings();
		break;
	}
	case StartupState::DisabledByPolicy:
	{
		MessageBoxW(
			hWnd,
			L"Disabled by policy. Contact your administrator.",
			L"Startup unavailable",
			MB_OK | MB_ICONWARNING
		);

		CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
		EnableWindow(GetDlgItem(hWnd, IDM_CHECKBOX), FALSE);
		break;
	}
	default: break;
	}

	RefreshStartupUI(hWnd);
}

//  Processes messages for the main window.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	const int id = LOWORD(wParam);
	const int code = HIWORD(wParam);

	switch (message) {
	case WM_APP_STARTUP_STATE_READY:
	{
		g_startupEnableInFlight.store(false);
		SetWindowTextW(GetDlgItem(hWnd, IDM_CHECKBOX), L"Start with Windows");
		RefreshStartupUI(hWnd);
		break;
	}
	case WM_TIMER:
	{
		//if (wParam == IDT_MY && timer.IsRunning()) { /* ... */ }
		//RefreshWindow(hWnd);
		break;	
	}
	case WM_SYSTEM_TRAY_ICON: {
		switch (LOWORD(lParam))
		{
		case WM_LBUTTONDBLCLK: OnLButtonDoubleClick(hWnd);                          break;
		case WM_CONTEXTMENU:   OnContextMenu(hWnd, LOWORD(wParam), HIWORD(wParam)); break;
		}
		break;
	}
	case WM_KEYDOWN:
		if (id == VK_ESCAPE) {
			OnClose(hWnd);
			return 0;
		}
		break;
	case WM_COMMAND:
	{
		switch (id)
		{
		case IDM_ABOUT:
			OnFileAbout(hWnd, hInst);
			break;
		case IDM_EXIT:
			OnFileExit(hWnd);
			break;
		case IDM_DISABLE:
			OnDisable(hWnd, id, code);
			break;
		case IDM_CHECKBOX:
			if (code == BN_CLICKED)
				HandleStartupCheckbox(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	}
	case WM_PAINT:
		OnPaint(hWnd);
		break;
	case WM_ACTIVATEAPP:
		if (wParam && !g_startupEnableInFlight.load())
		{
			g_startupManager.EnsureInitializedAsync(hWnd); // refresh state
			RefreshStartupUI(hWnd);
			RefreshWindow(hWnd);
		}
		break;
	case WM_CREATE:
	{
		hCheckbx = CreateWindow(
			TEXT("button"),
			TEXT("Start with Windows"),
			WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
			20, 40, 185, 35,
			hWnd, (HMENU)IDM_CHECKBOX,
			((LPCREATESTRUCT)lParam)->hInstance,
			NULL);

		g_startupManager.EnsureInitializedAsync(hWnd);   // Starting background initialization
		RefreshStartupUI(hWnd);
		OnCreate(hWnd);
		break;
	}
	case WM_DESTROY:
		OnDestroy(hWnd);
		break;
	case WM_CLOSE:
		OnClose(hWnd);
		break;
	case WM_CTLCOLORSTATIC:
		// set transparent color checkbox
		if (hCheckbx == (HWND)lParam)
		{
			SetBkMode((HDC)wParam, TRANSPARENT);
			return (LRESULT)::GetStockObject(NULL_BRUSH);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}

	return (INT_PTR)FALSE;
}

static bool IsKeyPressed(int vKey)
{
	return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}

static bool HandleKeyEvent(const KeyEvent& e)
{
	// ignore external input
	if (e.injected) return false;

	// "Disable" menu item — pass-through everything
	if (g_state.m_disableChecked) return false;

	// ignore KeyUp (we react only on press)
	if (!e.isKeyDown) return false;

	// ignore Alt-combos (as before)
	if (e.altDown) return false;

	if (!g_tLayout || !g_tUI)
		return false;

	if (e.vkCode == VK_CAPITAL)
	{
		if (e.isShiftDown)
		{
			SwitchToLayoutNumber(1);
			g_state.m_isLayoutDefault = false;
			g_tLayout->Reschedule(kLayoutIdleTimeoutMs);
		}
		else
		{
			SwitchToLayoutNumber(0);
			g_state.m_isLayoutDefault = true;
			g_tLayout->Stop();
		}

		g_tUI->Reschedule(kUI_IdleTimeoutMs);

		return true; // block CapsLock (do not toggle system state)
	}

	if (!g_state.m_isLayoutDefault)
		g_tLayout->Reschedule(kLayoutIdleTimeoutMs);

	return false;
}

static bool HandleTimerEvent(const StateEvent& e)
{
	if (!g_tLayout || !g_tUI)
		return false;

	switch (e.timerType) {
	case TimerType::LayoutSwitch:
	{
		SwitchToLayoutNumber(0);
		g_tLayout->Stop();
		g_state.m_isLayoutDefault = true;
		break;
	}
	case TimerType::UI:
	{
		RefreshWindow(e.hWnd);
		g_tUI->Stop();
		break;
	}
	default:;
	}

	return true;
}