#pragma once
#include <Windows.h>
#include <appmodel.h>	// GetCurrentPackageFullName, APPMODEL_ERROR_NO_PACKAGE

namespace platform {

inline bool IsPackagedProcess()
{
	UINT32 len = 0;
	const LONG rc = GetCurrentPackageFullName(&len, nullptr);
	return (rc == ERROR_INSUFFICIENT_BUFFER || rc == ERROR_SUCCESS);
}

} // namespace platform