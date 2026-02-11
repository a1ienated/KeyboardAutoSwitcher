#include "stdafx.h"
#include "IniSettingsStore.h"
#include <windows.h>
#include <cwchar>
#include <string>
#include <optional>
#include "Platform/Packaging.h"
#include "Platform/StoragePaths.h"

#include <winrt/Windows.Storage.h>

static std::wstring ReadIniString(const std::wstring& path, const wchar_t* section, const wchar_t* key)
{
	wchar_t buf[1024] = {};
	DWORD n = GetPrivateProfileStringW(section, key, L"", buf, (DWORD)std::size(buf), path.c_str());
	return (n > 0) ? std::wstring(buf, n) : L"";
}

static bool ParseBool(const std::wstring& s, bool def)
{
	if (s.empty()) return def;
	if (s == L"1" || s == L"true" || s == L"TRUE" || s == L"True") return true;
	if (s == L"0" || s == L"false" || s == L"FALSE" || s == L"False") return false;
	return def;
}

static std::optional<uint32_t> ParseU32(const std::wstring& s)
{
	if (s.empty()) return std::nullopt;
	wchar_t* end = nullptr;
	unsigned long v = std::wcstoul(s.c_str(), &end, 10);
	if (end == s.c_str()) return std::nullopt;
	return (uint32_t)v;
}

IniSettingsStore::IniSettingsStore(std::wstring iniPath)
	: m_path(std::move(iniPath))
{
}

std::wstring IniSettingsStore::DefaultPath()
{
	if (platform::IsPackagedProcess())
	{
		try // LocalState/settings.ini inside the package
		{
			auto folder = winrt::Windows::Storage::ApplicationData::Current().LocalFolder();
			std::wstring base = folder.Path().c_str();
			if (!base.empty() && base.back() != L'\\') base += L'\\';
			return base + L"settings.ini";
		}
		catch (...)
		{
			return platform::storage::GetAppDataDir() + L"\\settings.ini";
		}
	}
	else
		return platform::storage::GetAppDataDir() + L"\\settings.ini";
}

void IniSettingsStore::Init(std::wstring iniPath)
{
	m_path = std::move(iniPath);
}

AppSettings IniSettingsStore::Load()
{
	AppSettings s{};

	// version
	if (auto v = ParseU32(ReadIniString(m_path, Section, SettingsKeys::Version)))
		s.settings_version = *v;

	// enabled
	s.enabled = ParseBool(ReadIniString(m_path, Section, SettingsKeys::Enabled), s.enabled);

	// timeout_ms
	auto timeoutMsStr = ReadIniString(m_path, Section, SettingsKeys::TimeoutMs);
	if (auto v = ParseU32(timeoutMsStr))
		s.timeout_ms = *v;

	// UI_timeout_ms
	auto UI_TimeoutMsStr = ReadIniString(m_path, Section, SettingsKeys::UI_TimeoutMs);
	if (auto v = ParseU32(UI_TimeoutMsStr))
		s.UI_IdleTimeout_ms = *v;

	// layout_id
	auto lid = ReadIniString(m_path, Section, SettingsKeys::LayoutId);
	if (!lid.empty()) s.layout_id = lid;

	s.Normalize();
	return s;
}

bool IniSettingsStore::Save(const AppSettings& in)
{
	AppSettings s = in;
	s.Normalize();

	bool ok = true;

	ok = ok && (WritePrivateProfileStringW(Section, SettingsKeys::Version,
		std::to_wstring(s.settings_version).c_str(), m_path.c_str()) != 0);

	ok = ok && (WritePrivateProfileStringW(Section, SettingsKeys::Enabled,
		(s.enabled ? L"1" : L"0"), m_path.c_str()) != 0);

	ok = ok && (WritePrivateProfileStringW(Section, SettingsKeys::TimeoutMs,
		std::to_wstring(s.timeout_ms).c_str(), m_path.c_str()) != 0);

	ok = ok && (WritePrivateProfileStringW(Section, SettingsKeys::UI_TimeoutMs,
		std::to_wstring(s.UI_IdleTimeout_ms).c_str(), m_path.c_str()) != 0);

	ok = ok && (WritePrivateProfileStringW(Section, SettingsKeys::LayoutId,
		s.layout_id.c_str(), m_path.c_str()) != 0);

	return ok;
}
