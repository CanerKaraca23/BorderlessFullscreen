#pragma once
#include "misc.h"

class WindowedMode
{
public:
	static void InitGtaSA();

	// game internals
	GameState& gameState;
	RsGlobalTypeSA* rsGlobalSA;
	WNDPROC oriWindowProc;
	IDirect3DDevice8* &d3dDevice;
	D3DPRESENT_PARAMETERS* d3dPresentParams;
	DisplayMode** rwVideoModes; // array
	DWORD (*RwEngineGetNumVideoModes)();
	DWORD (*RwEngineGetCurrentVideoMode)();

	// constructor taking addresses for specific game version
	WindowedMode(
		uintptr_t gameState,
		uintptr_t rsGlobal,
		uintptr_t d3dDevice,
		uintptr_t d3dPresentParams,
		uintptr_t rwVideoModes,
		uintptr_t RwEngineGetNumVideoModes,
		uintptr_t RwEngineGetCurrentVideoMode
	);

	static HWND __stdcall InitWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	void InitD3dDevice(); // install hooks

	HWND window = 0;
	bool windowUpdating = false; // update in progress
	HICON windowIcon = NULL;
	char windowClassName[64];
	char windowTitle[64];
	// current window geometry
	POINT windowPos = {};
	POINT windowSize = {};
	POINT windowSizeClient = {};

	void WindowCalculateGeometry(bool resizeWindow = false);
	DWORD WindowStyle() const;
	DWORD WindowStyleEx() const;
	static LRESULT APIENTRY WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HRESULT static __stdcall D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters);
	decltype(D3dResetHook)* d3dResetOri;
};

static WindowedMode* inst; // global instance

