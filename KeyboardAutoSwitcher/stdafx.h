// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define WM_SYSTEM_TRAY_ICON  WM_USER + 5

// Windows Header Files:
#include <windows.h>
#include <shellapi.h>
#include <chrono>
#include <thread>
#include <mutex>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

