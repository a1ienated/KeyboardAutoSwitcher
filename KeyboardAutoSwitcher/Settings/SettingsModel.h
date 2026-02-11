#pragma once
#include <string>
#include <cstdint>

namespace SettingsKeys
{
    inline constexpr wchar_t Version[] = L"kas_settings_version";
    inline constexpr wchar_t Enabled[] = L"kas_enabled";
    inline constexpr wchar_t TimeoutMs[] = L"kas_timeout_ms";
    inline constexpr wchar_t UI_TimeoutMs[] = L"kas_UI_timeout_ms";
    inline constexpr wchar_t LayoutId[] = L"kas_layout_id";
}

struct AppSettings
{
    uint32_t settings_version = 1;

    bool enabled = true;
    uint32_t timeout_ms = 20000;
    uint32_t UI_IdleTimeout_ms = 50;
    std::wstring layout_id = L"system_default";

    void Normalize()
    {
        if (settings_version == 0) settings_version = 1;

        if (UI_IdleTimeout_ms < 50) UI_IdleTimeout_ms = 50; // UI refresh min 50ms
        if (timeout_ms > 60000) timeout_ms = 60000;   // 1 min max

        if (layout_id.empty()) layout_id = L"system_default";
    }
};

struct RuntimeState
{
    bool isLayoutDefault = true;
};

RuntimeState& Runtime();