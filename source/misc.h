#pragma once
#include <stdint.h>
#include <windows.h>
#include "injector/injector.hpp"
#include "injector/assembly.hpp"
#include "injector/calling.hpp"

// Minimal D3D type definitions
typedef DWORD D3DFORMAT;
typedef DWORD D3DMULTISAMPLE_TYPE;
typedef DWORD D3DSWAPEFFECT;

#define D3DFMT_A8R8G8B8 21
#define D3DSWAPEFFECT_DISCARD 1
#define D3DPRESENT_INTERVAL_DEFAULT 0

// Forward declarations
struct IDirectInputA;
struct IDirectInputDeviceA;
typedef IDirectInputA* LPDIRECTINPUT8;
typedef IDirectInputDeviceA* LPDIRECTINPUTDEVICE8;
struct IDirect3DDevice8;

// D3D present parameters
typedef struct _D3DPRESENT_PARAMETERS_
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
} D3DPRESENT_PARAMETERS;

struct RsPlatformSpecific
{
	HWND window;
	HINSTANCE instance;
	DWORD fullScreen;
	float lastMousePos[2];
	DWORD lastRenderTime;
	LPDIRECTINPUT8 diInterface;
	LPDIRECTINPUTDEVICE8 diMouse;
	LPDIRECTINPUTDEVICE8 diDevice1;
	LPDIRECTINPUTDEVICE8 diDevice2;
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

enum GameState : DWORD
{
	Startup,
	Init_Logo_Mpeg,
	Logo_Mpeg,
	Init_Intro_Mpeg,
	Intro_Mpeg,
	Init_Once,
	Init_Frontend,
	Frontend,
	Init_Playing_Game,
	Playing_Game,
};

struct DisplayMode
{
	UINT width;
	UINT height;
	UINT refreshRate;
	D3DFORMAT format;
	DWORD flags;
};

// RenderWare structures (minimal)
typedef float RwReal;
struct RwV2d { RwReal x, y; };
struct RwV3d { RwReal x, y, z; };
struct RwPlane { RwV3d normal; RwReal distance; };
struct RwFrustumPlane { RwPlane plane; uint8_t closestX, closestY, closestZ, pad; };
struct RwBBox { RwV3d sup, inf; };

struct RwRaster
{
	RwRaster* pParent;
	uint8_t* pPixels;
	uint8_t* pPalette;
	int32_t nWidth, nHeight, nDepth, nStride;
	int16_t nOffsetX, nOffsetY;
	uint8_t cType, cFlags, cPrivateFlags, cFormat;
	uint8_t* pOriginalPixels;
	int32_t nOriginalWidth, nOriginalHeight, nOriginalStride;
};

struct RwCamera
{
	char RwObjectHasFrame[20];
	uint32_t RwCameraProjection;
	uint32_t RwCameraBeginUpdateFunc;
	uint32_t RwCameraEndUpdateFunc;
	char RwMatrix[64];
	RwRaster* frameBuffer;
	RwRaster* zBuffer;
	RwV2d viewWindow, recipViewWindow, viewOffset;
	RwReal nearPlane, farPlane, fogPlane, zScale, zShift;
	RwFrustumPlane frustumPlanes[6];
	RwBBox frustumBoundBox;
	RwV3d frustumCorners[8];
};

// Helper functions
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

static inline bool HasFocus(HWND wnd)
{
	return GetForegroundWindow() == wnd;
}

static inline bool IsCursorInClientRect(HWND wnd)
{
	POINT pos;
	GetCursorPos(&pos);
	RECT rect;
	GetClientRect(wnd, &rect);
	ClientToScreen(wnd, (LPPOINT)&rect.left);
	ClientToScreen(wnd, (LPPOINT)&rect.right);
	return PtInRect(&rect, pos);
}

static inline bool IsKeyDown(int keyCode)
{
	return GetAsyncKeyState(keyCode) & 0x8000;
}

static inline std::string StringPrintf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	auto len = std::vsnprintf(nullptr, 0, format, args) + 1;
	va_end(args);
	std::string result(len, '\0');
	va_start(args, format);
	std::vsnprintf(result.data(), result.length(), format, args);
	va_end(args);
	return result;
}

static inline void SetCursorVisible(bool show)
{
	int targetState = show ? 0 : -1;
	int currState = ShowCursor(show);
	int tries = 128;
	while (currState != targetState && tries)
	{
		currState = ShowCursor(currState < targetState);
		tries--;
	}
}

template <class ... Args>
static void ShowError(const char* format, Args ... args)
{
	auto msg = StringPrintf(format, args...);
	auto wnd = GetActiveWindow();
	if (wnd)
	{
		PostMessage(wnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		ShowWindow(wnd, SW_MINIMIZE);
	}
	SetCursorVisible(true);
	MessageBoxA(wnd, msg.c_str(), rsc_ProductName, MB_SYSTEMMODAL | MB_ICONERROR);
}

static inline void VerifyMemory(const char* name, DWORD address, size_t size, DWORD expectedHash)
{
	auto buffer = std::vector<BYTE>(size);
	injector::ReadMemoryRaw(address, buffer.data(), size, true);
	
	DWORD crc = ~0;
	for (size_t i = 0; i < size; i++)
	{
		crc = crc ^ buffer.data()[i];
		for (size_t j = 0; j < 8; j++)
		{
			DWORD mask = ~(crc & 1) + 1;
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}
	crc = ~crc;

	if (crc != expectedHash)
	{
		ShowError("Memory \"%s\" modified!\nExpected hash: 0x%08X\nActual hash: 0x%08X", name, expectedHash, crc);
	}
}
