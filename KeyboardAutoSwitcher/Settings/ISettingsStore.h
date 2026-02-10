#pragma once
#include "SettingsModel.h"

class ISettingsStore
{
public:
    virtual ~ISettingsStore() = default;

    // return settings (with defaults).
    virtual AppSettings Load() = 0;

    // true/false — for log errors
    virtual bool Save(const AppSettings& s) = 0;
};