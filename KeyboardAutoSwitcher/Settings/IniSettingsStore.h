#pragma once
#include "ISettingsStore.h"
#include <string>

class IniSettingsStore final : public ISettingsStore
{
public:
	explicit IniSettingsStore(std::wstring iniPath);

	void Init(std::wstring iniPath);
	AppSettings Load() override;
	bool Save(const AppSettings& s) override;

	const std::wstring& Path() const noexcept { return m_path; }

	// path in LocalState (MSIX)
	static std::wstring DefaultPath();

private:
	std::wstring m_path;
	static constexpr const wchar_t* Section = L"settings";
};
