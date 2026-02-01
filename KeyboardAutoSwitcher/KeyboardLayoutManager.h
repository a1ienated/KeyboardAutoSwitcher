#pragma once
#include <wtypes.h>

#define MAX_LAYOUTS 5
#define MAX_NAME_LENGTH 1024

struct KeyboardLayoutInfo {
	WCHAR names[MAX_LAYOUTS][MAX_NAME_LENGTH];
	HKL   hkls[MAX_LAYOUTS];
	BOOL  inUse[MAX_LAYOUTS];
	UINT  count;
	UINT  currentSlotLang;
};

// Global Variables:
extern KeyboardLayoutInfo g_keyboardInfo;

void GetKeyboardLayouts();
UINT GetIndexLanguage();
HWND RemoteGetFocus();
HKL GetCurrentLayout();
void SwitchToLayoutNumber(int number);
HKL ChangeInputLanguage(UINT newLanguage);
void ResetKeyboardState();