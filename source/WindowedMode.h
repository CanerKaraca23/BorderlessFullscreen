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
	D3DPRESENT_PARAMETERS_D3D9* d3dPresentParams9;
	DisplayMode** rwVideoModes; // array
	DWORD (*RwEngineGetNumVideoModes)();
	DWORD (*RwEngineGetCurrentVideoMode)();
	uintptr_t frontEndMenuManager;

	// constructor taking addresses for specific game version
	WindowedMode(
		uintptr_t gameState,
		uintptr_t rsGlobal,
		uintptr_t d3dDevice,
		uintptr_t d3dPresentParams,
		uintptr_t rwVideoModes,
		uintptr_t RwEngineGetNumVideoModes,
		uintptr_t RwEngineGetCurrentVideoMode,
		uintptr_t frontEndMenuManager
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
	void WindowUpdateTitle();
	static LRESULT APIENTRY WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	POINT SizeFromClient(POINT clientSize) const;
	POINT ClientFromSize(POINT windowSize) const;
	RECT GetFrameSize(bool paddOnly = false) const; // size of window frame and extra padding/shadow introduced in later versions of Windows
	
	// DirectX 3D related stuff
	HRESULT static __stdcall D3dPresentHook(IDirect3DDevice8* self, const RECT* srcRect, const RECT* dstRect, HWND wnd, const RGNDATA* region);
	decltype(D3dPresentHook)* d3dPresentOri;

	HRESULT static __stdcall D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters);
	decltype(D3dResetHook)* d3dResetOri;

	// other
	void MouseUpdate(bool force = false);
	void UpdatePostEffect();
	void UpdateWidescreenFix();
};

static WindowedMode* inst; // global instance

