#include "stdafx.h"
#include "SettingsModel.h"

RuntimeState& Runtime()
{
    static RuntimeState s; // C++11+: thread-safety
    return s;
}