#include "WindowedMode.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		char name[128];
		injector::address_manager::singleton().GetVersionText(name);

		if (!_stricmp(name, "GTA SA 1.0.0.0 US"))
		{
			WindowedMode::InitGtaSA();
		}
		else
		{
			ShowError("Game version \"%s\" is not supported! \nOnly GTA San Andreas v1.0 US is supported.", name);
		}
	}

	return TRUE;
}