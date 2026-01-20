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
	constexpr auto kRsGlobalAddr = 0xC17040;
	constexpr auto kD3dDeviceAddr = 0xC97C28;
	constexpr auto kPresentParamsAddr = 0xC9C040;
	constexpr auto kRwVideoModesAddr = 0xC97C48;
	constexpr auto kRwEngineGetCurrentVideoModeAddr = 0x7F2D20;
}

static inline RECT GetMonitorRect(POINT pos)
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

class WindowedMode
{
public:
	static void InitGtaSA();

	RsGlobalTypeSA* rsGlobalSA;
	WNDPROC oriWindowProc;
	IDirect3DDevice8*& d3dDevice;
	D3DPRESENT_PARAMETERS* d3dPresentParams;
	DisplayMode** rwVideoModes;
	DWORD (*RwEngineGetCurrentVideoMode)();
	HICON windowIcon = nullptr;

	WindowedMode(
		uintptr_t rsGlobal,
		uintptr_t d3dDevice,
		uintptr_t d3dPresentParams,
		uintptr_t rwVideoModes,
		uintptr_t RwEngineGetCurrentVideoMode);

	static HWND __stdcall InitWindow(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
	void InitD3dDevice();
	void ApplyPresentationParams();

	HWND window = 0;
	bool windowUpdating = false;
	POINT windowPos = {};
	POINT windowSize = {};
	POINT windowSizeClient = {};

	void WindowCalculateGeometry(bool resizeWindow = false);
	DWORD WindowStyle() const;
	DWORD WindowStyleEx() const;
	static LRESULT APIENTRY WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HRESULT static __stdcall D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters);
	decltype(D3dResetHook)* d3dResetOri = nullptr;
};

static WindowedMode* inst = nullptr;

WindowedMode::WindowedMode(
	uintptr_t rsGlobal,
	uintptr_t d3dDevice,
	uintptr_t d3dPresentParams,
	uintptr_t rwVideoModes,
	uintptr_t RwEngineGetCurrentVideoMode)
	: rsGlobalSA(reinterpret_cast<RsGlobalTypeSA*>(rsGlobal)),
	oriWindowProc(nullptr),
	d3dDevice(*reinterpret_cast<IDirect3DDevice8**>(d3dDevice)),
	d3dPresentParams(reinterpret_cast<D3DPRESENT_PARAMETERS*>(d3dPresentParams)),
	rwVideoModes(reinterpret_cast<DisplayMode**>(rwVideoModes)),
	RwEngineGetCurrentVideoMode(reinterpret_cast<DWORD(*)()>(RwEngineGetCurrentVideoMode))
{}

HWND __stdcall WindowedMode::InitWindow(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE hInstance, LPVOID)
{
	WNDCLASSA oriClass;
	if (!GetClassInfo(hInstance, kWindowClassName, &oriClass))
	{
		return nullptr;
	}
	inst->oriWindowProc = oriClass.lpfnWndProc;

	inst->WindowCalculateGeometry();

	WNDCLASSA wndClass = {};
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = kWindowClassName;
	wndClass.hIcon = inst->windowIcon;
	wndClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpfnWndProc = &WindowedMode::WindowProc;

	UnregisterClass(kWindowClassName, hInstance);
	RegisterClass(&wndClass);

	inst->window = CreateWindowEx(
		inst->WindowStyleEx(),
		kWindowClassName,
		inst->rsGlobalSA->AppName,
		inst->WindowStyle(),
		inst->windowPos.x, inst->windowPos.y,
		inst->windowSize.x, inst->windowSize.y,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	inst->WindowCalculateGeometry(true);

	return inst->window;
}

void WindowedMode::InitD3dDevice()
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

DWORD WindowedMode::WindowStyle() const
{
	return WS_VISIBLE | WS_CLIPSIBLINGS | WS_POPUP;
}

DWORD WindowedMode::WindowStyleEx() const
{
	return 0;
}

void WindowedMode::ApplyPresentationParams()
{
	rsGlobalSA->ps->fullScreen = false;
	rsGlobalSA->ps->window = window;
	rsGlobalSA->MaximumWidth = windowSizeClient.x;
	rsGlobalSA->MaximumHeight = windowSizeClient.y;

	d3dPresentParams->Windowed = TRUE;
	d3dPresentParams->hDeviceWindow = window;
	d3dPresentParams->BackBufferWidth = windowSizeClient.x;
	d3dPresentParams->BackBufferHeight = windowSizeClient.y;
	d3dPresentParams->BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dPresentParams->SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresentParams->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dPresentParams->FullScreen_RefreshRateInHz = 0;
}

void WindowedMode::WindowCalculateGeometry(bool resizeWindow)
{
	windowUpdating = true;

	POINT windowCenter = { windowPos.x + windowSize.x / 2, windowPos.y + windowSize.y / 2 };
	auto monitorRect = GetMonitorRect(windowCenter);
	auto monitorWidth = monitorRect.right - monitorRect.left;
	auto monitorHeight = monitorRect.bottom - monitorRect.top;

	windowPos.x = monitorRect.left;
	windowPos.y = monitorRect.top;
	windowSize.x = windowSizeClient.x = monitorWidth;
	windowSize.y = windowSizeClient.y = monitorHeight;

	if (resizeWindow && window)
	{
		SetWindowLong(window, GWL_STYLE, WindowStyle());
		SetWindowLong(window, GWL_EXSTYLE, WindowStyleEx());
		SetWindowPos(window, HWND_TOP, windowPos.x, windowPos.y, windowSize.x, windowSize.y,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}

	ApplyPresentationParams();

	if (*rwVideoModes)
	{
		auto& mode = (*rwVideoModes)[RwEngineGetCurrentVideoMode()];
		mode.width = windowSizeClient.x;
		mode.height = windowSizeClient.y;
		mode.format = d3dPresentParams->BackBufferFormat;
		mode.refreshRate = d3dPresentParams->FullScreen_RefreshRateInHz;
		mode.flags &= ~1;
	}

	windowUpdating = false;
}

LRESULT APIENTRY WindowedMode::WindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	case WM_WINDOWPOSCHANGED:
		if (!inst->windowUpdating)
			inst->WindowCalculateGeometry(true);
		break;
	}

	return CallWindowProc(inst->oriWindowProc, wnd, msg, wParam, lParam);
}

HRESULT WindowedMode::D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters)
{
	(void)parameters; // unused in hook; we use stored params instead
	inst->WindowCalculateGeometry();
	return inst->d3dResetOri(self, inst->d3dPresentParams);
}

void WindowedMode::InitGtaSA()
{
	inst = new WindowedMode(
		kRsGlobalAddr,
		kD3dDeviceAddr,
		kPresentParamsAddr,
		kRwVideoModesAddr,
		kRwEngineGetCurrentVideoModeAddr
	);
	inst->windowIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(100));

	injector::MakeNOP(0x7455D5, 6);
	injector::MakeCALL(0x7455D5, WindowedMode::InitWindow);

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
		WindowedMode::InitGtaSA();
	}

	return TRUE;
}
