#pragma once
#include <string>
#include <Windows.h>
#include <cstdarg>

namespace logger
{
    std::wstring Now();
    std::string WideToUtf8(const std::wstring& w);
    void AppendFileUtf8(const std::wstring& path, const std::wstring& line);
    std::wstring HResultToText(HRESULT hr);
    void WriteV(const wchar_t* level, const wchar_t* fmt, va_list ap);
    void Info(const wchar_t* fmt, ...);
    void Error(const wchar_t* fmt, ...);
    void Hr(const wchar_t* where, HRESULT hr);
}
