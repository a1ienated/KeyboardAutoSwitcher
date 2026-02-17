#pragma once
#include <string>
#include <Windows.h>
#include <ShlObj.h>
#include <KnownFolders.h>
#include "Platform/Packaging.h"

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

	inline std::wstring GetLocalStateDir()
	{
		if (!platform::IsPackagedProcess())
			return GetAppDataDir();

		// base: C:\Users\<User>\AppData\Local
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

		UINT32 len = 0;
		LONG rc = GetCurrentPackageFamilyName(&len, nullptr);
		if (rc != ERROR_INSUFFICIENT_BUFFER)
			return GetAppDataDir(); // fallback

		std::wstring pfn;
		pfn.resize(len);
		rc = GetCurrentPackageFamilyName(&len, pfn.data());
		if (rc != ERROR_SUCCESS)
			return GetAppDataDir();

		if (!pfn.empty() && pfn.back() == L'\0') pfn.pop_back();

		// C:\Users\<User>\AppData\Local\Packages\<PFN>\LocalState
		std::wstring dir = base + L"\\Packages\\" + pfn + L"\\LocalState";

		CreateDirectoryW(dir.c_str(), nullptr);

		return dir;
	}

} // namespace platform::storage