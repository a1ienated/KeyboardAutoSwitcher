#include "stdafx.h"
#include "KeyboardAutoSwitcher.h"
#include "KeyboardLayoutManager.h"
#include "StartupManager.h"
#include "KeyboardHook.h"
#include<tchar.h>
#include<xstring>

#include <atomic>
#include <thread>
#include <winrt/Windows.ApplicationModel.h>

typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>>String;

#define MAX_LOADSTRING 100
#define MAX_LINE 3
#define MAX_CHAR 100
#define IDT_TIMER1 1
#define IDT_TIMER2 2
#define IDT_TIMER3 3
#define ONE_SECOND 1000

#define MAX_LOADSTRING 100
#define STRLEN(x) (sizeof(x)/sizeof(TCHAR) - 1)

// global var
UINT countTimerSec = 0;
BOOL isAnyPressed = FALSE;
BOOL mDisableChecked = FALSE;

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hCheckbx;

wchar_t info[MAX_LINE][MAX_CHAR];
NOTIFYICONDATA nid = {};

static KeyboardHook g_keyboardHook;

static std::atomic_bool g_startupEnableInFlight{ false };
static StartupManager g_startupManager;

// Forward declarations of functions included in this code module:
ATOM RegMyWindowClass(HINSTANCE hInst, LPCTSTR lpzClassName);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime);
static bool HandleKeyEvent(const KeyEvent& e);
void SetTimerCount(unsigned int count);

// main window
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Prevent from two copies of Recaps from running at the same time
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
	// Set hook to capture keyboard events
	g_keyboardHook.SetCallback(HandleKeyEvent);
	if (!g_keyboardHook.Install()) {
		KAS_DLOGF(L"[Hook] Failed to install WH_KEYBOARD_LL hook. err=%lu", GetLastError());
	}
	PostMessage(NULL, WM_NULL, 0, 0);

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

	if (!hWnd) return FALSE;

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
		ShowWindow(hWnd, SW_HIDE);
		ShowWindow(hWnd, SW_RESTORE);
//		ShowWindow(hWnd, SW_SHOW);
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
	mDisableChecked = !mDisableChecked;
	MENUITEMINFO mi = { 0 };
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_STATE;
	mi.fState = mDisableChecked ? MF_CHECKED : MF_UNCHECKED;
	SetMenuItemInfo(hSubMenu, 1, TRUE, &mi);

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

	g_keyboardInfo.currentSlotLang = GetIndexLanguage();
	TCHAR labelLayout[50] = _T("Language: ");
	_tcscat(labelLayout, g_keyboardInfo.names[g_keyboardInfo.currentSlotLang]);
	TextOut(hDC, 20, 30, labelLayout, _tcsclen(labelLayout));

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
	wcscpy_s(info[2], MAX_CHAR, L"30 sec timer - on last input switches to default layout");

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
	KillTimer(hWnd, IDT_TIMER1);
	KillTimer(hWnd, IDT_TIMER2);

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

void SetTimerCount(unsigned int count)
{
	if (count == 0)
	{
		countTimerSec--;
		return;
	}

	countTimerSec = count;
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
	switch (message) {

	case WM_APP_STARTUP_STATE_READY:
	{
		g_startupEnableInFlight.store(false);
		SetWindowTextW(GetDlgItem(hWnd, IDM_CHECKBOX), L"Start with Windows");
		RefreshStartupUI(hWnd);
		break;
	}
	case WM_TIMER: {
		if (isAnyPressed) {
			isAnyPressed = false;
			SetTimer(hWnd, IDT_TIMER1, ONE_SECOND * 15, NULL);
			SetTimerCount(10);
		}
		else
			SwitchToLayoutNumber(0);
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
		if (wParam == VK_ESCAPE) {
			OnClose(hWnd);
			return 0;
		}
		break;
	case WM_COMMAND:
	{
		const int id = LOWORD(wParam);
		const int code = HIWORD(wParam);

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

		SetTimer(hWnd, IDT_TIMER1, ONE_SECOND * 15, NULL);
		SetTimer(hWnd, IDT_TIMER2, ONE_SECOND, (TIMERPROC)TimerProc);
		SetTimerCount(10);
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

// MyTimerProc is an application-defined callback function that processes WM_TIMER messages.
VOID CALLBACK TimerProc(
	HWND hWnd,        // handle to window for timer messages
	UINT message,     // WM_TIMER message
	UINT idTimer,     // timer identifier
	DWORD dwTime)     // current system time
{
	if (countTimerSec > 0)
		SetTimerCount(0);

	InvalidateRect(hWnd, NULL, TRUE);
}

static bool IsKeyPressed(int vKey)
{
	return (GetAsyncKeyState(vKey) & 0x8000) != 0;
}

static bool HandleKeyEvent(const KeyEvent& e)
{
	// "Disable" menu item — pass-through everything
	if (mDisableChecked)
		return false;

	// ignore KeyUp (we react only on press)
	if (!e.isKeyDown)
		return false;

	// ignore Alt-combos (as before)
	if (e.altDown)
		return false;

	isAnyPressed = true;

	if (e.vkCode == VK_CAPITAL)
	{
		if (IsKeyPressed(VK_SHIFT))
			SwitchToLayoutNumber(1);
		else
			SwitchToLayoutNumber(0);

		return true; // block CapsLock (do not toggle system state)
	}

	return false;
}
