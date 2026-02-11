#include "stdafx.h"
#include "SettingsModel.h"

RuntimeState& Runtime()
{
	static RuntimeState s;
	return s;
}