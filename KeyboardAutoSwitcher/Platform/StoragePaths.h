#pragma once
#include <string>
#include <Windows.h>
#include <ShlObj.h>
#include <KnownFolders.h>

#pragma comment(lib, "Shell32.lib")

namespace platform::storage {

	inline std::wstring GetAppDataDir()
	{
		PWSTR p = nullptr;
		std::wstring base;

		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &p)) && p)
		{
			base = p;
			CoTaskMemFree(p);
		}
		else
		{
			wchar_t tmp[MAX_PATH]{};
			GetTempPathW(MAX_PATH, tmp);
			base = tmp;
		}

		std::wstring dir = base + L"\\KeyboardAutoSwitcher";
		CreateDirectoryW(dir.c_str(), nullptr); // ок, if already exist

		return dir;
	}

} // namespace platform::storage