#pragma once

#include <windows.h>
#include <strsafe.h>
#include <cstdarg>
#include <string>
#include <intrin.h> // __debugbreak

#ifdef _DEBUG
// ------------------------- Helpers -------------------------
namespace kas::dbg {

    inline void Print(const wchar_t* msg) noexcept {
        ::OutputDebugStringW(msg);
    }

    inline void PrintLine(const wchar_t* msg) noexcept {
        ::OutputDebugStringW(msg);
        ::OutputDebugStringW(L"\n");
    }

    inline void Printf(const wchar_t* fmt, ...) noexcept {
        wchar_t buf[2048]{};
        va_list args;
        va_start(args, fmt);
        (void)::StringCchVPrintfW(buf, _countof(buf), fmt, args);
        va_end(args);
        ::OutputDebugStringW(buf);
    }

    inline std::wstring FormatWin32Error(DWORD err) {
        if (err == 0) return L"";

        wchar_t* sysMsg = nullptr;
        DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS;

        DWORD len = ::FormatMessageW(
            flags, nullptr, err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&sysMsg, 0, nullptr);

        std::wstring out;
        if (len && sysMsg) {
            out.assign(sysMsg, sysMsg + len);
            // remove tail \r\n, if exist
            while (!out.empty() && (out.back() == L'\n' || out.back() == L'\r' || out.back() == L' '))
                out.pop_back();
        }
        if (sysMsg) ::LocalFree(sysMsg);
        return out;
    }

    inline void LogLastError(const wchar_t* call, const wchar_t* file, int line) noexcept {
        DWORD err = ::GetLastError();
        std::wstring msg = FormatWin32Error(err);
        Printf(L"[KAS][WIN32] %s failed @%s:%d | GetLastError=%lu (%s)\n",
            call, file, line, err, msg.c_str());
    }

    inline void AssertFail(const wchar_t* expr, const wchar_t* file, int line) noexcept {
        Printf(L"[KAS][ASSERT] %s @%s:%d\n", expr, file, line);
    }

    inline int MsgBox(HWND owner, const wchar_t* text, const wchar_t* title, UINT flags) noexcept {
        return ::MessageBoxW(owner, text ? text : L"", title ? title : L"", flags);
    }

    inline int MsgBoxF(HWND owner, const wchar_t* title, UINT flags, const wchar_t* fmt, ...) noexcept {
        wchar_t buf[2048]{};
        va_list args;
        va_start(args, fmt);
        (void)::StringCchVPrintfW(buf, _countof(buf), fmt, args);
        va_end(args);
        return ::MessageBoxW(owner, buf, title ? title : L"", flags);
    }

    struct ScopeTimer {
        const wchar_t* name{};
        LARGE_INTEGER t0{};
        LARGE_INTEGER freq{};

        explicit ScopeTimer(const wchar_t* n) : name(n) {
            ::QueryPerformanceFrequency(&freq);
            ::QueryPerformanceCounter(&t0);
        }
        ~ScopeTimer() {
            LARGE_INTEGER t1{};
            ::QueryPerformanceCounter(&t1);
            const double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)freq.QuadPart;
            Printf(L"[KAS][TIMER] %s = %.3f ms\n", name, ms);
        }
    };

} // namespace kas::dbg


// ------------------------- MACROS -------------------------



// It is possible to set this in the project properties, for example: KAS_DBG_MASK=0x3 (UI+HOOK)
#ifndef KAS_DBG_MASK
#define KAS_DBG_MASK 0xFFFFFFFFu
#endif

enum : unsigned {
    KAS_DBG_UI = 1u << 0,
    KAS_DBG_HOOK = 1u << 1,
    KAS_DBG_TIMER = 1u << 2,
    KAS_DBG_STATE = 1u << 3,
};

// Log as a string (literal)
#define KAS_DLOG(msgLiteral) \
    do { ::kas::dbg::PrintLine(L"[KAS] " msgLiteral); } while (0)

// Log format
#define KAS_DLOGF(fmt, ...) \
    do { ::kas::dbg::Printf(L"[KAS] " fmt L"\n", __VA_ARGS__); } while (0)

// Log by category (literal)
#define KAS_DLOG_CAT(mask, msgLiteral) \
    do { if ((KAS_DBG_MASK & (mask)) != 0) ::kas::dbg::PrintLine(L"[KAS] " msgLiteral); } while (0)

// Log by category (format)
#define KAS_DLOGF_CAT(mask, fmt, ...) \
    do { if ((KAS_DBG_MASK & (mask)) != 0) ::kas::dbg::Printf(L"[KAS] " fmt L"\n", __VA_ARGS__); } while (0)

// Assert / Verify
#define KAS_ASSERT(expr) \
    do { if (!(expr)) { ::kas::dbg::AssertFail(L#expr, _CRT_WIDE(__FILE__), __LINE__); __debugbreak(); } } while (0)

#define KAS_VERIFY(expr) KAS_ASSERT(expr)

// For WinAPI: if the call returned FALSE/NULL, etc., log GetLastError.
#define KAS_WIN32(call) \
    do { if (!(call)) { ::kas::dbg::LogLastError(L#call, _CRT_WIDE(__FILE__), __LINE__); } } while (0)

// A simple message
#define KAS_MSG(hwnd, textLiteral) \
    do { ::kas::dbg::MsgBox((hwnd), (textLiteral), L"KAS (Debug)", MB_OK | MB_ICONINFORMATION); } while (0)

// Message with header/flags
#define KAS_MSG_EX(hwnd, titleLiteral, flags, textLiteral) \
    do { ::kas::dbg::MsgBox((hwnd), (textLiteral), (titleLiteral), (flags)); } while (0)

// Formatted message
#define KAS_MSGF(hwnd, fmt, ...) \
    do { ::kas::dbg::MsgBoxF((hwnd), L"KAS (Debug)", MB_OK | MB_ICONINFORMATION, (fmt), __VA_ARGS__); } while (0)

// Formatted + header/flags
#define KAS_MSGF_EX(hwnd, titleLiteral, flags, fmt, ...) \
    do { ::kas::dbg::MsgBoxF((hwnd), (titleLiteral), (flags), (fmt), __VA_ARGS__); } while (0)

// Scope timer
#define KAS_SCOPE_TIMER(nameLiteral) ::kas::dbg::ScopeTimer _kas_scope_timer_##__LINE__(nameLiteral)

// ------------------------- Examples of use -------------------------
/*

KAS_DLOG(L"App started");

KAS_DLOGF(L"Layouts count=%u current=%u", g_keyboardInfo.count, g_keyboardInfo.currentSlotLang);

KAS_DLOG_CAT(KAS_DBG_HOOK, L"Hook installed");

KAS_DLOGF_CAT(KAS_DBG_UI, L"WM_COMMAND id=%u", (unsigned)LOWORD(wParam));

KAS_ASSERT(g_hHook != NULL);

KAS_VERIFY(RegisterHotKey(hwnd, 1, MOD_ALT, 'K')); // It won't crash in the Release, but the call will remain.

KAS_WIN32(CreateDirectoryW(L"C:\\Temp\\kas", nullptr)); // If it fails, it will print an error message.

KAS_MSG(hwnd, L"Hook installed!");

KAS_MSG_EX(hwnd, L"Warning", MB_OK | MB_ICONWARNING, L"Something looks off");

KAS_MSGF(hwnd, L"Layouts=%u, current=%u", g_keyboardInfo.count, g_keyboardInfo.currentSlotLang);

KAS_MSGF_EX(hwnd, L"Error", MB_OK | MB_ICONERROR,
    L"RegisterHotKey failed, err=%lu", GetLastError());

{
    KAS_SCOPE_TIMER(L"WndProc");
    // ... do something ...
}
*/
#else

#define KAS_DLOG(...)            ((void)0)
#define KAS_DLOGF(...)           ((void)0)
#define KAS_DLOG_CAT(...)        ((void)0)
#define KAS_DLOGF_CAT(...)       ((void)0)
#define KAS_ASSERT(expr)         ((void)0)
#define KAS_VERIFY(expr)         ((void)(expr))
#define KAS_WIN32(call)          ((void)(call))
#define KAS_SCOPE_TIMER(...)     ((void)0)
#define KAS_MSG(...)        ((void)0)
#define KAS_MSG_EX(...)     ((void)0)
#define KAS_MSGF(...)       ((void)0)
#define KAS_MSGF_EX(...)    ((void)0)

#endif
