#include "stdafx.h"
#include "logger.h"
#include "Platform/StoragePaths.h"

#include <comdef.h>
#include <mutex>
#include <cstdarg>
#include <vector>
#include <ShlObj.h>

#pragma comment(lib, "Shell32.lib")

namespace logger
{
    static std::mutex g_mtx;

    std::wstring Now()
    {
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t buf[64];
        swprintf_s(buf, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        return buf;
    }

    std::string WideToUtf8(const std::wstring& w)
    {
        if (w.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
        std::string out(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), len, nullptr, nullptr);
        return out;
    }

    void AppendFileUtf8(const std::wstring& path, const std::wstring& line)
    {
        HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return;

        std::string utf8 = WideToUtf8(line + L"\r\n");
        DWORD written = 0;
        WriteFile(h, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
        CloseHandle(h);
    }

    std::wstring HResultToText(HRESULT hr)
    {
        wchar_t hexbuf[32];
        swprintf_s(hexbuf, L"0x%08X", (unsigned)hr);

        _com_error err(hr);
        const wchar_t* msg = err.ErrorMessage();
        std::wstring text = msg ? msg : L"";

        if (!text.empty())
            return std::wstring(hexbuf) + L" (" + text + L")";

        return hexbuf;
    }

    void WriteV(const wchar_t* level, const wchar_t* fmt, va_list ap)
    {
        wchar_t buf[2048];
        vswprintf_s(buf, fmt, ap);

        std::wstring line = Now() + L" [" + level + L"] " + buf;

        // Debug output
        OutputDebugStringW((line + L"\n").c_str());

        // File
        AppendFileUtf8(platform::storage::GetLocalStateDir() + L"\\kas.log", line);
    }

    void Info(const wchar_t* fmt, ...)
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        va_list ap; va_start(ap, fmt);
        WriteV(L"INFO", fmt, ap);
        va_end(ap);
    }

    void Error(const wchar_t* fmt, ...)
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        va_list ap; va_start(ap, fmt);
        WriteV(L"ERROR", fmt, ap);
        va_end(ap);
    }

    void Hr(const wchar_t* where, HRESULT hr)
    {
        Error(L"%s: %s", where, HResultToText(hr).c_str());
    }
}
