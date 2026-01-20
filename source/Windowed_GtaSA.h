#include "WindowedMode.h"
#include "injector/assembly.hpp"

void WindowedMode::InitGtaSA()
{
	inst = new WindowedMode(
		0xC8D4C0, // gameState
		0xC17040, // rsGlobal
		0xC97C28, // d3dDevice
		0xC9C040, // d3dPresentParams
		0xC97C48, // rwVideoModes
		0x7F2CC0, // RwEngineGetNumVideoModes
		0x7F2D20  // RwEngineGetCurrentVideoMode
	);

	strcpy_s(inst->windowClassName, "Grand theft auto San Andreas");
	inst->windowIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(100));

	// patch call to CreateWindowExA
	injector::MakeNOP(0x7455D5, 6);
	injector::MakeCALL(0x7455D5, WindowedMode::InitWindow);

	struct Patch_InitPresentationParams // just before D3D device is created
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(DWORD*)(0xC97C4C); // original action replaced by the patch

			inst->WindowCalculateGeometry();
		}
	}; injector::MakeInline<Patch_InitPresentationParams>(0x7F670A, 0x7F6710);

	struct Path_InitD3dDevice // just after D3D device has been created
	{
		void operator()(injector::reg_pack& regs)
		{
			*(DWORD*)(0xC9808C) = regs.ebp; // original action replaced by the patch

			inst->InitD3dDevice();
		}
	}; injector::MakeInline<Path_InitD3dDevice>(0x7F6800, 0x7F6806);
}
