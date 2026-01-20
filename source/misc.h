#pragma once
#include <stdint.h>
#include <windows.h>
#include <string>
#include <cstdarg>
#include <cstdio>

#include "injector/injector.hpp"

// Minimal D3D type definitions
typedef DWORD D3DFORMAT;
typedef DWORD D3DMULTISAMPLE_TYPE;
typedef DWORD D3DSWAPEFFECT;

#define D3DFMT_A8R8G8B8 21
#define D3DSWAPEFFECT_DISCARD 1
#define D3DPRESENT_INTERVAL_DEFAULT 0

struct IDirect3DDevice8;
struct RsPlatformSpecific;

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
