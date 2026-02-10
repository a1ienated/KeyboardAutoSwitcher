#pragma once
#include <string>
#include <cstdint>

namespace SettingsKeys
{
    inline constexpr wchar_t Version[] = L"kas_settings_version";
    inline constexpr wchar_t Enabled[] = L"kas_enabled";
    inline constexpr wchar_t TimeoutMs[] = L"kas_timeout_ms";
    inline constexpr wchar_t LayoutId[] = L"kas_layout_id";
}

struct AppSettings
{
    uint32_t settings_version = 1;
    bool enabled = true;
    
    // minimal settings + defaults
    uint32_t timeout_ms = 20000;
    std::wstring layout_id = L"system_default";

    void Normalize()
    {
        if (settings_version == 0) settings_version = 1;

        if (timeout_ms < 50 || timeout_ms > 50) timeout_ms = 50; // 50ms
        if (timeout_ms > 60000) timeout_ms = 60000;   // 1 min max

        if (layout_id.empty()) layout_id = L"system_default";
    }
};

// don't save
struct RuntimeState
{
    bool isLayoutDefault = true;
};

RuntimeState& Runtime();