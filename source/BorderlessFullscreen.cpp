// Single-file minimal borderless fullscreen plugin for GTA SA 1.0 US
#include <windows.h>

#include "injector/injector.hpp"
#include "injector/assembly.hpp"

using D3DFORMAT = DWORD;
using D3DMULTISAMPLE_TYPE = DWORD;
using D3DSWAPEFFECT = DWORD;

constexpr D3DFORMAT D3DFMT_A8R8G8B8 = 21;
constexpr D3DSWAPEFFECT D3DSWAPEFFECT_DISCARD = 1;
constexpr DWORD D3DPRESENT_INTERVAL_DEFAULT = 0;

struct IDirect3DDevice8;

struct RsPlatformSpecific
{
	HWND window;
	HINSTANCE instance;
	DWORD fullScreen;
};

struct RsGlobalTypeSA
{
	char* AppName;
	int MaximumWidth;
	int MaximumHeight;
	int frameLimit;
	int quit;
	RsPlatformSpecific* ps;
};

struct DisplayMode
{
	UINT width;
	UINT height;
	UINT refreshRate;
	D3DFORMAT format;
	DWORD flags;
};

struct D3DPRESENT_PARAMETERS
{
	UINT BackBufferWidth;
	UINT BackBufferHeight;
	D3DFORMAT BackBufferFormat;
	UINT BackBufferCount;
	D3DMULTISAMPLE_TYPE MultiSampleType;
	DWORD MultiSampleQuality;
	D3DSWAPEFFECT SwapEffect;
	HWND hDeviceWindow;
	BOOL Windowed;
	BOOL EnableAutoDepthStencil;
	D3DFORMAT AutoDepthStencilFormat;
	DWORD Flags;
	UINT FullScreen_RefreshRateInHz;
	UINT FullScreen_PresentationInterval;
};

namespace
{
	constexpr const char* kWindowClassName = "Grand theft auto San Andreas";
	constexpr int kGameIconResource = 100;
	constexpr auto kRsGlobalAddr = 0xC17040;
	constexpr auto kD3dDeviceAddr = 0xC97C28;
	constexpr auto kPresentParamsAddr = 0xC9C040;
	constexpr auto kRwVideoModesAddr = 0xC97C48;
	constexpr auto kRwEngineGetCurrentVideoModeAddr = 0x7F2D20;
}

static inline RECT GetMonitorRect(const POINT pos)
{
	auto monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info = { sizeof(MONITORINFO) };
	if (!GetMonitorInfo(monitor, &info))
	{
		monitor = MonitorFromPoint(pos, MONITOR_DEFAULTTOPRIMARY);
		GetMonitorInfo(monitor, &info);
	}
	return info.rcMonitor;
}

class BorderlessMode
{
public:
	static void InitGtaSA();

	RsGlobalTypeSA* rsGlobalSA;
	WNDPROC originalWindowProc;
	IDirect3DDevice8*& d3dDevice;
	D3DPRESENT_PARAMETERS* d3dPresentParams;
	DisplayMode** rwVideoModes;
	DWORD (*RwEngineGetCurrentVideoMode)();
	HICON windowIcon = nullptr;

	BorderlessMode(
		uintptr_t rsGlobal,
		uintptr_t d3dDevice,
		uintptr_t d3dPresentParams,
		uintptr_t rwVideoModes,
		uintptr_t RwEngineGetCurrentVideoMode);

	static HWND __stdcall InitWindow(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
	void InitD3dDevice();
	void ApplyPresentationParams();

	HWND window = nullptr;
	bool isUpdating = false;

	void WindowCalculateGeometry(bool resizeWindow = false);
	static LRESULT APIENTRY WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HRESULT static __stdcall D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters);
	decltype(D3dResetHook)* d3dResetOri = nullptr;
};

static BorderlessMode* inst = nullptr;

BorderlessMode::BorderlessMode(
	uintptr_t rsGlobal,
	uintptr_t d3dDevice,
	uintptr_t d3dPresentParams,
	uintptr_t rwVideoModes,
	uintptr_t RwEngineGetCurrentVideoMode)
	: rsGlobalSA(reinterpret_cast<RsGlobalTypeSA*>(rsGlobal)),
	originalWindowProc(nullptr),
	d3dDevice(*reinterpret_cast<IDirect3DDevice8**>(d3dDevice)),
	d3dPresentParams(reinterpret_cast<D3DPRESENT_PARAMETERS*>(d3dPresentParams)),
	rwVideoModes(reinterpret_cast<DisplayMode**>(rwVideoModes)),
	RwEngineGetCurrentVideoMode(reinterpret_cast<DWORD(*)()>(RwEngineGetCurrentVideoMode))
{}

HWND __stdcall BorderlessMode::InitWindow(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE hInstance, LPVOID)
{
	WNDCLASSA oriClass;
	if (!GetClassInfo(hInstance, kWindowClassName, &oriClass))
	{
		return nullptr;
	}
	inst->originalWindowProc = oriClass.lpfnWndProc;

	inst->WindowCalculateGeometry();

	WNDCLASSA wndClass = {};
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = kWindowClassName;
	wndClass.hIcon = inst->windowIcon;
	wndClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpfnWndProc = &BorderlessMode::WindowProc;

	UnregisterClass(kWindowClassName, hInstance);
	RegisterClass(&wndClass);

	inst->window = CreateWindowEx(
		0,
		kWindowClassName,
		inst->rsGlobalSA->AppName,
		WS_VISIBLE | WS_CLIPSIBLINGS | WS_POPUP,
		0, 0, 0, 0,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	inst->WindowCalculateGeometry(true);

	return inst->window;
}

void BorderlessMode::InitD3dDevice()
{
	if (d3dDevice == nullptr)
	{
		return;
	}

	auto vTable = *(uintptr_t**)d3dDevice;
	DWORD oldProtect;
	injector::UnprotectMemory(vTable, 18 * sizeof(uintptr_t), oldProtect);

	d3dResetOri = reinterpret_cast<decltype(d3dResetOri)>(vTable[16]);
	vTable[16] = (uintptr_t)&D3dResetHook;
}

void BorderlessMode::ApplyPresentationParams()
{
	rsGlobalSA->ps->fullScreen = false;
	rsGlobalSA->ps->window = window;

	d3dPresentParams->Windowed = TRUE;
	d3dPresentParams->hDeviceWindow = window;
	d3dPresentParams->BackBufferWidth = rsGlobalSA->MaximumWidth;
	d3dPresentParams->BackBufferHeight = rsGlobalSA->MaximumHeight;
	d3dPresentParams->BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dPresentParams->SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresentParams->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dPresentParams->FullScreen_RefreshRateInHz = 0;
}

void BorderlessMode::WindowCalculateGeometry(bool resizeWindow)
{
	isUpdating = true;

	RECT windowRect;
	if (!GetWindowRect(window, &windowRect))
	{
		windowRect = {};
	}

	const POINT windowCenter = { (windowRect.left + windowRect.right) / 2, (windowRect.top + windowRect.bottom) / 2 };
	const auto monitorRect = GetMonitorRect(windowCenter);
	const auto width = monitorRect.right - monitorRect.left;
	const auto height = monitorRect.bottom - monitorRect.top;

	rsGlobalSA->MaximumWidth = width;
	rsGlobalSA->MaximumHeight = height;

	if (resizeWindow && window)
	{
		SetWindowLong(window, GWL_STYLE, WS_VISIBLE | WS_CLIPSIBLINGS | WS_POPUP);
		SetWindowLong(window, GWL_EXSTYLE, 0);
		SetWindowPos(window, HWND_TOP, monitorRect.left, monitorRect.top, width, height,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}

	ApplyPresentationParams();

	if (*rwVideoModes)
	{
		auto& mode = (*rwVideoModes)[RwEngineGetCurrentVideoMode()];
		mode.width = rsGlobalSA->MaximumWidth;
		mode.height = rsGlobalSA->MaximumHeight;
		mode.format = d3dPresentParams->BackBufferFormat;
		mode.refreshRate = d3dPresentParams->FullScreen_RefreshRateInHz;
		mode.flags &= ~1;
	}

	isUpdating = false;
}

LRESULT APIENTRY BorderlessMode::WindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	case WM_WINDOWPOSCHANGED:
		if (!inst->isUpdating)
			inst->WindowCalculateGeometry(true);
		break;
	}

	return CallWindowProc(inst->originalWindowProc, wnd, msg, wParam, lParam);
}

HRESULT BorderlessMode::D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters)
{
	(void)parameters; // unused in hook; we use stored params instead
	inst->WindowCalculateGeometry();
	return inst->d3dResetOri(self, inst->d3dPresentParams);
}

void BorderlessMode::InitGtaSA()
{
	inst = new BorderlessMode(
		kRsGlobalAddr,
		kD3dDeviceAddr,
		kPresentParamsAddr,
		kRwVideoModesAddr,
		kRwEngineGetCurrentVideoModeAddr
	);
	inst->windowIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(kGameIconResource));

	injector::MakeNOP(0x7455D5, 6);
	injector::MakeCALL(0x7455D5, BorderlessMode::InitWindow);

	struct Patch_InitPresentationParams
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = *(DWORD*)(0xC97C4C);
			inst->WindowCalculateGeometry();
		}
	}; injector::MakeInline<Patch_InitPresentationParams>(0x7F670A, 0x7F6710);

	struct Patch_InitD3dDevice
	{
		void operator()(injector::reg_pack& regs)
		{
			*(DWORD*)(0xC9808C) = regs.ebp;
			inst->InitD3dDevice();
		}
	}; injector::MakeInline<Patch_InitD3dDevice>(0x7F6800, 0x7F6806);
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		BorderlessMode::InitGtaSA();
	}

	return TRUE;
}
