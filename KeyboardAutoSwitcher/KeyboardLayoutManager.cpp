#include "stdafx.h"
#include "KeyboardLayoutManager.h"

KeyboardLayoutInfo g_keyboardInfo = {};

void GetKeyboardLayouts() {
	memset(&g_keyboardInfo, 0, sizeof(KeyboardLayoutInfo));
	g_keyboardInfo.count = GetKeyboardLayoutList(MAX_LAYOUTS, g_keyboardInfo.hkls);

	for (UINT i = 0; i < g_keyboardInfo.count; i++) {
		LANGID language = (LANGID)(((UINT_PTR)g_keyboardInfo.hkls[i]) & 0x0000FFFF); // bottom 16 bit of HKL
		LCID locale = MAKELCID(language, SORT_DEFAULT);
		GetLocaleInfo(locale, LOCALE_SLANGUAGE, g_keyboardInfo.names[i], MAX_NAME_LENGTH);
		g_keyboardInfo.inUse[i] = TRUE;
	}

	UINT idx = GetIndexLanguage();
	if (idx >= g_keyboardInfo.count) idx = 0;   // fallback if the current HKL is not in the first MAX_LAYOUTS or count==0
	g_keyboardInfo.currentSlotLang = idx;
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

HKL ChangeInputLanguage(UINT newLanguage)
{
	HWND hwnd = RemoteGetFocus();
	g_keyboardInfo.currentSlotLang = newLanguage;
	PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)(g_keyboardInfo.hkls[g_keyboardInfo.currentSlotLang]));

	if (g_keyboardInfo.count == 0) return GetCurrentLayout();
	if (newLanguage >= g_keyboardInfo.count) return GetCurrentLayout();
	return g_keyboardInfo.hkls[g_keyboardInfo.currentSlotLang];
}

void ResetKeyboardState()
{
	ZeroMemory(&g_keyboardInfo, sizeof(g_keyboardInfo));
}