#include "stdafx.h"
#include "UIPresenter.h"

#include <tchar.h>

namespace UIPresenter
{
	// ---- internal state ----
	static UINT  g_dpi = USER_DEFAULT_SCREEN_DPI; // 96
	static HFONT g_fontBold = nullptr;
	static HWND  g_checkbox = nullptr;

	static constexpr int kLabelX = 20;
	static constexpr int kLabelY = 10;

	static constexpr int kCheckboxX = 20;
	static constexpr int kCheckboxY = 40;
	static constexpr int kCheckboxW = 185;
	static constexpr int kCheckboxH = 35;

	static constexpr int kLinesX = 20;
	static constexpr int kLinesY = 80;

	static constexpr int kExtraPt = 1;

	static int Scale(int dip)
	{
		return MulDiv(dip, (int)g_dpi, USER_DEFAULT_SCREEN_DPI);
	}

	UINT CurrentDpi() { return g_dpi; }
	int  Px(int dip)  { return Scale(dip); }

	static void RecreateBoldFont(UINT dpi)
	{
		g_dpi = dpi;

		if (g_fontBold) {
			DeleteObject(g_fontBold);
			g_fontBold = nullptr;
		}

		// system UI font, but correct for a specific DPI
		NONCLIENTMETRICS ncm{};
		ncm.cbSize = sizeof(ncm);

		SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0, dpi);

		LOGFONT lf = ncm.lfMessageFont;
		// +kExtraPt pt to the base UI font
		const int addPx = MulDiv(kExtraPt, (int)dpi, 72);
		if (lf.lfHeight < 0) lf.lfHeight -= addPx; // negative -> greater in magnitude -> greater in size
		else				lf.lfHeight += addPx;

		lf.lfWeight = FW_BOLD;

		g_fontBold = CreateFontIndirect(&lf);
	}

	static void ApplyFonts()
	{
		if (g_checkbox && g_fontBold)
			SendMessage(g_checkbox, WM_SETFONT, (WPARAM)g_fontBold, TRUE);
	}

	static void LayoutCheckbox()
	{
		if (!g_checkbox) return;

		SetWindowPos(
			g_checkbox, nullptr,
			Scale(kCheckboxX), Scale(kCheckboxY),
			Scale(kCheckboxW), Scale(kCheckboxH),
			SWP_NOZORDER | SWP_NOACTIVATE
		);
	}

	RECT CalcInitialWindowRectDips(
		UINT clientW_dip,
		UINT clientH_dip,
		DWORD style,
		DWORD exStyle,
		bool hasMenu,
		UINT dpi)
	{
		RECT rc{ 0, 0, (LONG)MulDiv((int)clientW_dip, (int)dpi, 96), (LONG)MulDiv((int)clientH_dip, (int)dpi, 96) };
		AdjustWindowRectExForDpi(&rc, style, hasMenu ? TRUE : FALSE, exStyle, dpi);
		return rc;
	}

	void InitForWindow(HWND hWnd)
	{
		RecreateBoldFont(GetDpiForWindow(hWnd));
		ApplyFonts();
		LayoutCheckbox();
		InvalidateRect(hWnd, nullptr, TRUE);
	}

	void AttachCheckbox(HWND hCheckbox)
	{
		g_checkbox = hCheckbox;
		ApplyFonts();
		LayoutCheckbox();
	}

	void OnDpiChanged(HWND hWnd, WPARAM wParam, LPARAM lParam)
	{
		const UINT newDpi = LOWORD(wParam);

		RecreateBoldFont(newDpi);
		ApplyFonts();
		LayoutCheckbox();

		// recommended rect from Windows
		const RECT* prc = reinterpret_cast<const RECT*>(lParam);
		SetWindowPos(
			hWnd, nullptr,
			prc->left, prc->top,
			prc->right - prc->left,
			prc->bottom - prc->top,
			SWP_NOZORDER | SWP_NOACTIVATE
		);

		InvalidateRect(hWnd, nullptr, TRUE);
	}

	void Paint(HWND hWnd, const PaintModel& model)
	{
		PAINTSTRUCT ps{};
		HDC hdc = BeginPaint(hWnd, &ps);

		SetBkMode(hdc, TRANSPARENT);

		HFONT useFont = g_fontBold ? g_fontBold : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		HFONT old = (HFONT)SelectObject(hdc, useFont);

		// Title
		if (model.title && model.title[0]) {
			TextOutW(hdc, Scale(kLabelX), Scale(kLabelY), model.title, (int)wcslen(model.title));
		}

		// Lines
		int x = Scale(kLinesX);
		int y = Scale(kLinesY);

		TEXTMETRIC tm{};
		GetTextMetrics(hdc, &tm);
		int d = tm.tmHeight + tm.tmExternalLeading;

		for (int i = 0; i < model.lineCount; ++i) {
			const wchar_t* s = model.lines ? model.lines[i] : nullptr;
			if (!s) continue;
			TextOutW(hdc, x, y, s, (int)wcslen(s));
			y += d;
		}

		SelectObject(hdc, old);
		EndPaint(hWnd, &ps);
	}

	void Shutdown()
	{
		if (g_fontBold) {
			DeleteObject(g_fontBold);
			g_fontBold = nullptr;
		}
		g_checkbox = nullptr;
		g_dpi = USER_DEFAULT_SCREEN_DPI;
	}
}
