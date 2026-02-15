#pragma once
#include <windows.h>

namespace UIPresenter
{
	constexpr int MAX_LINE = 3;
	constexpr int MAX_CHAR = 100;

	struct PaintModel
	{
		const wchar_t* title = nullptr;
		const wchar_t* const* lines = nullptr;
		int lineCount = 0;
	};

	// Client sizes in DIPs (96 DPI)
	RECT CalcInitialWindowRectDips(
		UINT clientW_dip,
		UINT clientH_dip,
		DWORD style,
		DWORD exStyle,
		bool hasMenu,
		UINT dpi);

	void InitForWindow(HWND hWnd);
	void AttachCheckbox(HWND hCheckbox);

	void OnDpiChanged(HWND hWnd, WPARAM wParam, LPARAM lParam);

	// BeginPaint/EndPaint inside
	void Paint(HWND hWnd, const PaintModel& model);

	void Shutdown();

	// Utils
	UINT CurrentDpi();
	int  Px(int dip);
}
