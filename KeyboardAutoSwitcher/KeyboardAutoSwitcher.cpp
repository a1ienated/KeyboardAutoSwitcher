#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include "KeyboardAutoSwitcher.h"
#include<tchar.h>
#include<xstring>
typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>>String;

#define MAX_LOADSTRING 100
#define MAX_LINE 3
#define MAX_CHAR 100
#define IDT_TIMER1 1
#define IDT_TIMER2 2
#define IDT_TIMER3 3
#define ONE_SECOND 1000

#define MAX_LOADSTRING 100
#define MAX_LAYOUTS 5
#define MAX_NAME_LENGTH 1024
#define STRLEN(x) (sizeof(x)/sizeof(TCHAR) - 1)

// Global Variables:
struct KeyboardLayoutInfo {
	WCHAR names[MAX_LAYOUTS][MAX_NAME_LENGTH];
	HKL   hkls[MAX_LAYOUTS];
	BOOL  inUse[MAX_LAYOUTS];
	UINT  count;
	UINT  currentSlotLang;
}g_keyboardInfo;
UINT countTimerSec = 0;
HHOOK g_hHook = NULL;
BOOL isAnyPressed = FALSE;
BOOL mDisableChecked = FALSE;
std::mutex mtx;

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hCheckbx;

wchar_t info[MAX_LINE][MAX_CHAR];
NOTIFYICONDATA nid = {};

// Forward declarations of functions included in this code module:
ATOM RegMyWindowClass(HINSTANCE hInst, LPCTSTR lpzClassName);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
VOID CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime);
LRESULT CALLBACK LowLevelHookProc(int nCode, WPARAM wParam, LPARAM lParam);
void GetKeyboardLayouts();
void SwitchToLayoutNumber(int number);
void SetTimerCount(unsigned int count);
UINT GetIndexLanguage();

// main window
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	// Prevent from two copies of Recaps from running at the same time
	HANDLE mutex = CreateMutex(NULL, FALSE, L"KeyboardAutoSwitcher");
	if (!mutex || WaitForSingleObject(mutex, 0) == WAIT_TIMEOUT) {
		MessageBox(NULL, L"KeyboardAutoSwitcher is already running.", L"KeyboardAutoSwitcher", MB_OK | MB_ICONINFORMATION);
		return 1;
	}
    
	// Store instance handle in our global variable.
    hInst = hInstance;

	GetKeyboardLayouts();
	// Set hook to capture CapsLock
	g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelHookProc, GetModuleHandle(NULL), 0);
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

    return (int) msg.wParam;
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

void OnLButtonDoubleClick(HWND hWnd)
{
//	if (IsWindowVisible(hWnd) == TRUE)
	{
		ShowWindow(hWnd, SW_HIDE);
		ShowWindow(hWnd, SW_RESTORE);
//		ShowWindow(hWnd, SW_SHOW);
	}

//	ShowWindow(hWnd, SW_SHOW);
}

// show pop-up menu
void OnContextMenu(HWND hWnd, int x, int y)
{
	HMENU hMenu = GetMenu(hWnd);
	HMENU hSubMenu = GetSubMenu(hMenu, 0);

	TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, x, y, 0, hWnd, NULL);
}

void OnDisable(HWND hWnd, int x, int y)
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
void OnPaint(HWND hWnd)
{
//	TCHAR strCount[10], strTm[20] = _T("Timer: ");
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

//  	_tcscat(strTm, _itot(countTimerSec, strCount, 10));
//  	TextOut(hDC, 20, 10, strTm, _tcsclen(strTm));
	
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

void OnCreate(HWND hWnd)
{
	wcscpy_s(info[0], MAX_CHAR, L"Caps Lock - default layout");
	wcscpy_s(info[1], MAX_CHAR, L"Shift + Caps Lock - second layout");
	wcscpy_s(info[2], MAX_CHAR, L"30 sec timer - on last input switches to default layout");

	// add icon in system tray
    nid.cbSize           = sizeof(nid);
    nid.hWnd             = hWnd;
	nid.uID              = IDI_KEYBOARDAUTOSWITCHER;
    nid.uFlags           =  NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
	nid.hIcon            = LoadIcon(hInst, MAKEINTRESOURCE(IDI_KEYBOARDAUTOSWITCHER));
    nid.uCallbackMessage = WM_SYSTEM_TRAY_ICON;
	nid.uVersion         = NOTIFYICON_VERSION_4;
	wcscpy_s(nid.szTip,128,L"KeyboardAutoSwitcher - portable");

	Shell_NotifyIcon(NIM_ADD, &nid);
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

void OnDestroy(HWND hWnd)
{
	KillTimer(hWnd, IDT_TIMER1);
	KillTimer(hWnd, IDT_TIMER2);

	// remove icon from system tray
	Shell_NotifyIcon(NIM_DELETE, &nid);
	PostQuitMessage(0);
}

void OnClose(HWND hWnd)
{
	ShowWindow(hWnd, SW_HIDE);
}

// display about dialog box
void OnFileAbout(HWND hWnd, HINSTANCE hInstance)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, DlgProc);
}

void OnFileExit(HWND hWnd)
{
	DestroyWindow(hWnd);
}

void SetTimerCount(unsigned int count)
{
//	std::lock_guard<std::mutex> lock(mtx);
	if (count == 0)
	{
		countTimerSec--;
		return;
	}

	countTimerSec = count;
}

//  Processes messages for the main window.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_TIMER:
		if (isAnyPressed) {
			isAnyPressed = false;
			SetTimer(hWnd, IDT_TIMER1, ONE_SECOND * 15, NULL);
			SetTimerCount(10);
		}
		else
			SwitchToLayoutNumber(0);
		break;
	case WM_SYSTEM_TRAY_ICON:
		switch (LOWORD(lParam))
		{
			case WM_LBUTTONDBLCLK: OnLButtonDoubleClick(hWnd);                          break;
			case WM_CONTEXTMENU:   OnContextMenu(hWnd, LOWORD(wParam), HIWORD(wParam)); break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDM_ABOUT: 
			OnFileAbout(hWnd, hInst);
			break;
		case IDM_EXIT:  
			OnFileExit(hWnd);
			break;
		case IDM_DISABLE:
			OnDisable(hWnd, LOWORD(wParam), HIWORD(wParam));
			break;
		case IDM_CHECKBOX:
		{
			BOOL checked = IsDlgButtonChecked(hWnd, IDM_CHECKBOX);
			if (checked) {
				// RegDelKey();
				CheckDlgButton(hWnd, IDM_CHECKBOX, BST_UNCHECKED);
			}
			else {
				// RegAddKey();
				CheckDlgButton(hWnd, IDM_CHECKBOX, BST_CHECKED);
			}
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		OnPaint(hWnd);
		break;
	case WM_CREATE:
		hCheckbx = CreateWindow(TEXT("button"),
			TEXT("Start with Windows"),
			WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
			20, 40, 185, 35,
			hWnd, (HMENU)IDM_CHECKBOX, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		// CheckDlgButton(hWnd, IDM_CHECKBOX, RegFindKey() ? BST_CHECKED : BST_UNCHECKED);

		SetTimer(hWnd, IDT_TIMER1, ONE_SECOND * 15, NULL);
		SetTimer(hWnd, IDT_TIMER2, ONE_SECOND, (TIMERPROC)TimerProc);
		SetTimerCount(10);
		OnCreate(hWnd);
		break;
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

HWND RemoteGetFocus() 
{
	HWND hwnd = GetForegroundWindow();
	DWORD remoteThreadId = GetWindowThreadProcessId(hwnd, NULL);
	DWORD currentThreadId = GetCurrentThreadId();

	AttachThreadInput(remoteThreadId, currentThreadId, TRUE);
	HWND focused = GetFocus();
	AttachThreadInput(remoteThreadId, currentThreadId, FALSE);

	if (!focused) return hwnd;
	return focused;
}

HKL GetCurrentLayout() 
{
	HWND hwnd = RemoteGetFocus();
	DWORD threadId = GetWindowThreadProcessId(hwnd, NULL);
	return GetKeyboardLayout(threadId);
}

UINT GetIndexLanguage()
{
	HKL currentLayout = GetCurrentLayout();
	UINT index = 0;
	for (; index < g_keyboardInfo.count; index++) {
		if (!g_keyboardInfo.inUse[index]) {
			continue;
		}
		if (g_keyboardInfo.hkls[index] == currentLayout)
			break;
	}
	return index;
}

void GetKeyboardLayouts() {
	memset(&g_keyboardInfo, 0, sizeof(KeyboardLayoutInfo));
	g_keyboardInfo.count = GetKeyboardLayoutList(MAX_LAYOUTS, g_keyboardInfo.hkls);
	for (UINT i = 0; i < g_keyboardInfo.count; i++) {
		LANGID language = (LANGID)(((UINT)g_keyboardInfo.hkls[i]) & 0x0000FFFF); // bottom 16 bit of HKL
		LCID locale = MAKELCID(language, SORT_DEFAULT);
		GetLocaleInfo(locale, LOCALE_SLANGUAGE, g_keyboardInfo.names[i], MAX_NAME_LENGTH);
		g_keyboardInfo.inUse[i] = TRUE;
	}

	g_keyboardInfo.currentSlotLang = GetIndexLanguage();
}

HKL ChangeInputLanguage(UINT newLanguage) 
{
	HWND hwnd = RemoteGetFocus();
	g_keyboardInfo.currentSlotLang = newLanguage;
	PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)(g_keyboardInfo.hkls[g_keyboardInfo.currentSlotLang]));
	return g_keyboardInfo.hkls[g_keyboardInfo.currentSlotLang];
}

void SwitchToLayoutNumber(int number) 
{
	HKL currentLayout = GetCurrentLayout();
	for (UINT i = 0; i < g_keyboardInfo.count; i++) {
		if (!g_keyboardInfo.inUse[i]) {
			continue;
		}
		if (number == i) {
			if (g_keyboardInfo.hkls[i] != currentLayout)
				ChangeInputLanguage(i);
			break;
		}
	}
}

BOOL IsKeyPressed(int nVirtKey)
{
	return (GetKeyState(nVirtKey) & 0x80000000) > 0;
}

LRESULT CALLBACK LowLevelHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0 || mDisableChecked) return CallNextHookEx(g_hHook, nCode, wParam, lParam);

	KBDLLHOOKSTRUCT* data = (KBDLLHOOKSTRUCT*)lParam;

	if ((data->flags & LLKHF_ALTDOWN) == 0)
	{
		//WM_KEYDOWN WM_KEYUP WM_SYSKEYDOWN WM_SYSKEYUP
		if (wParam == WM_KEYDOWN)
		{
			isAnyPressed = true;

			switch (data->vkCode) {
			case VK_CAPITAL:
				if (IsKeyPressed(VK_SHIFT))
					SwitchToLayoutNumber(1);
				else
					SwitchToLayoutNumber(0);

				return 1; // prevent windows from handling the keystroke
			default:
				break;
			}
		}
	}

	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

