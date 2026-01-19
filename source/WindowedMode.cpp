#include "WindowedMode.h"
#include "Windowed_GtaSA.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib") // DwmGetWindowAttribute
#pragma comment(lib, "winmm.lib") // timeGetTime

// list of popular display aspect ratios
const WindowedMode::AspectRatioInfo WindowedMode::AspectRatios[] = {
	{ "1:1", 1.0f / 1.0f },
	{ "2:1", 2.0f / 1.0f },
	{ "4:3", 4.0f / 3.0f },
	{ "5:4", 5.0f / 4.0f },
	{ "10:7", 10.0f / 7.0f }, // GTA default aspect
	{ "16:9", 16.0f / 9.0f },
	{ "16:10", 16.0f / 10.0f }
};

WindowedMode::WindowedMode(
	uintptr_t gameState,
	uintptr_t rsGlobal,
	uintptr_t d3dDevice,
	uintptr_t d3dPresentParams,
	uintptr_t rwVideoModes,
	uintptr_t RwEngineGetNumVideoModes,
	uintptr_t RwEngineGetCurrentVideoMode,
	uintptr_t frontEndMenuManager
) :
	gameState(*(GameState*)gameState),
	rsGlobalSA((RsGlobalTypeSA*)rsGlobal),
	d3dDevice(*(IDirect3DDevice8**)d3dDevice),
	d3dPresentParams9((D3DPRESENT_PARAMETERS_D3D9*)d3dPresentParams),
	rwVideoModes((DisplayMode**)rwVideoModes),
	RwEngineGetNumVideoModes(*(DWORD(*)())RwEngineGetNumVideoModes),
	RwEngineGetCurrentVideoMode(*(DWORD(*)())RwEngineGetCurrentVideoMode),
	frontEndMenuManager(frontEndMenuManager)
{}

HWND __stdcall WindowedMode::InitWindow(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	WNDCLASSA oriClass;
	if (!GetClassInfo(hInstance, inst->windowClassName, &oriClass))
	{
		ShowError("Game window class not found!");
		return NULL;
	}
	inst->oriWindowProc = oriClass.lpfnWndProc;

	inst->InitConfig();
	inst->LoadConfig();

	inst->WindowCalculateGeometry();
	inst->WindowUpdateTitle();

	WNDCLASSA wndClass;
	wndClass.hInstance = hInstance;
	wndClass.lpszClassName = inst->windowClassName;
	wndClass.style = 0;
	wndClass.hIcon = inst->windowIcon;
	wndClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wndClass.lpszMenuName = NULL;
	wndClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	wndClass.lpfnWndProc = &WindowedMode::WindowProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	
	UnregisterClass(inst->windowClassName, hInstance);
	RegisterClass(&wndClass);

	inst->window = CreateWindowEx(
		inst->WindowStyleEx(),
		inst->windowClassName,
		inst->windowTitle,
		inst->WindowStyle(),
		inst->windowPos.x, inst->windowPos.y,
		inst->windowSize.x, inst->windowSize.y,
		NULL, // parent
		NULL, // menu
		hInstance,
		0);

	inst->WindowCalculateGeometry(true); // now modern styles border padding can be calculated

	UpdateWindow(inst->window);
	inst->MouseUpdate(true); // lock cursor in the window until main menu appears

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
	injector::UnprotectMemory(vTable, 20 * sizeof(uintptr_t), oldProtect);

	// DirectX 9 hooks
	d3dResetOri = reinterpret_cast<decltype(d3dResetOri)>(vTable[16]);
	vTable[16] = (uintptr_t)&D3dResetHook;

	d3dPresentOri = reinterpret_cast<decltype(d3dPresentOri)>(vTable[17]);
	vTable[17] = (uintptr_t)&D3dPresentHook;
}

void WindowedMode::InitConfig()
{
	auto attr = GetFileAttributes(config.GetIniPath().string().c_str());
	
	if (attr == INVALID_FILE_ATTRIBUTES) // does not exists
		SaveConfig();
}

bool WindowedMode::LoadConfig()
{
	menuFrameRateLimit = config.ReadInteger("game", "menuFPS", 30);
	autoPause = config.ReadInteger("game", "autoPause", true) != false;
	autoResume = config.ReadInteger("game", "autoResume", true) != false;

	return false;
}

void WindowedMode::SaveConfig()
{
	config.WriteString("game", "menuFPS",		StringPrintf("%d\t\t; frame rate limit for main menu. 0: unlimited", menuFrameRateLimit));
	config.WriteString("game", "autoPause",		StringPrintf("%d\t\t; pause the game on window deactivation", autoPause));
	config.WriteString("game", "autoResume",	StringPrintf("%d\t; resume the game on window activation", autoResume));
}

int WindowedMode::FindAspectRatio(POINT resolution, float treshold)
{
	auto ratio = float(resolution.x) / resolution.y;

	// find best match in
	int idx = -1;
	float dist = 9999.0f;
	for (size_t i = 0; i < _countof(AspectRatios); i++)
	{
		auto diff = fabs(AspectRatios[i].ratio - ratio);
		if (diff < dist)
		{
			idx = i;
			dist = diff;
		}
	}

	return dist <= treshold ? idx : -1;
}

DWORD WindowedMode::WindowStyle() const
{
	return WS_VISIBLE | WS_CLIPSIBLINGS | WS_POPUP;
}

DWORD WindowedMode::WindowStyleEx() const
{
	return 0;
}

void WindowedMode::WindowCalculateGeometry(bool resizeWindow)
{
	windowUpdating = true;

	// Get primary monitor (default to center of screen if window not yet created)
	POINT windowCenter = { windowPos.x + windowSize.x / 2, windowPos.y + windowSize.y / 2};
	auto monitorRect = GetMonitorRect(windowCenter);
	auto monitorWidth = monitorRect.right - monitorRect.left;
	auto monitorHeight = monitorRect.bottom - monitorRect.top;

	// Borderless fullscreen - always use full monitor size at monitor position
	windowPos.x = monitorRect.left;
	windowPos.y = monitorRect.top;
	windowSize.x = windowSizeClient.x = monitorWidth;
	windowSize.y = windowSizeClient.y = monitorHeight;

	// apply to the window
	if (resizeWindow)
	{
		SetWindowLong(window, GWL_STYLE, WindowStyle());
		SetWindowLong(window, GWL_EXSTYLE, WindowStyleEx());
		SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_FRAMECHANGED | SWP_SHOWWINDOW); // update the frame
		auto padding = GetFrameSize(true);

		SetWindowPos(window, 0,
			windowPos.x - padding.left,
			windowPos.y - padding.top,
			windowSize.x,
			windowSize.y,
			SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_SHOWWINDOW);

		WindowUpdateTitle();
	}

	// apply resolution to game internals
	rsGlobalSA->ps->fullScreen = false;
	rsGlobalSA->ps->window = window;
	rsGlobalSA->MaximumWidth = windowSizeClient.x;
	rsGlobalSA->MaximumHeight = windowSizeClient.y;

	// DirectX 9 parameters
	d3dPresentParams9->Windowed = TRUE;
	d3dPresentParams9->hDeviceWindow = window;
	d3dPresentParams9->BackBufferWidth = windowSizeClient.x;
	d3dPresentParams9->BackBufferHeight = windowSizeClient.y;
	d3dPresentParams9->BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dPresentParams9->SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dPresentParams9->FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
	d3dPresentParams9->FullScreen_RefreshRateInHz = 0;

	// write the resolution into video display modes list
	if (*rwVideoModes)
	{
		// backup display modes infos before making any changes
		static std::vector<DisplayMode> videoModesBackup;
		if (videoModesBackup.empty())
		{
			auto count = RwEngineGetNumVideoModes();
			videoModesBackup.resize(count);
			memcpy(videoModesBackup.data(), *rwVideoModes, count * sizeof(DisplayMode));
		}

		static int prevVideoMode = -1;
		int currVideoMode = RwEngineGetCurrentVideoMode();
	
		// restore previous display mode info to its original state
		if (prevVideoMode != -1 && currVideoMode != prevVideoMode) (*rwVideoModes)[prevVideoMode] = videoModesBackup[prevVideoMode];
		prevVideoMode = currVideoMode;

		// write modified resolution into current display mode info
		auto& mode = (*rwVideoModes)[currVideoMode];
		mode.width = windowSizeClient.x;
		mode.height = windowSizeClient.y;
		mode.format = d3dPresentParams9->BackBufferFormat;
		mode.refreshRate = d3dPresentParams9->FullScreen_RefreshRateInHz;
		mode.flags &= ~1; // clear fullscreen flag
	}

	windowUpdating = false;
}

void WindowedMode::WindowUpdateTitle()
{
	if (HasFocus(window))
	{
		std::string aspectTxt;
		auto idx = FindAspectRatio(windowSizeClient);
		if (idx != -1)
		{
			aspectTxt = StringPrintf(" (%s)", AspectRatios[idx].name);
		}

		sprintf_s(windowTitle, "%s | %ux%u%s @ %u fps",
			rsGlobalSA->AppName,
			windowSizeClient.x,
			windowSizeClient.y,
			aspectTxt.c_str(),
			fpsCounter.get());
	}
	else
		strcpy_s(windowTitle, rsGlobalSA->AppName);
	
	if (window)
		SetWindowText(window, windowTitle);
}

LRESULT APIENTRY WindowedMode::WindowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// window focus/defocus
		case WM_ACTIVATE:
		{
			auto result = (LOWORD(wParam) == WA_INACTIVE) ?
				DefWindowProc(wnd, msg, wParam, lParam) : // don't pause game on defocus
				CallWindowProc(inst->oriWindowProc, wnd, msg, wParam, lParam);

			// handle automatic pause/resume
			if (inst->gameState == Playing_Game)
			{
				switch(LOWORD(wParam))
				{
					case WA_INACTIVE:
						if (inst->autoPause && !inst->IsMainMenuVisible())
						{
							inst->SwitchMainMenu(true);
							inst->autoPauseExecuted = true;
						}
						break;

					case WA_CLICKACTIVE: // mouse click
						if (!IsCursorInClientRect(wnd))
						{
							inst->autoPauseExecuted = false;
							break; // user clicked on the window caption or edge
						}
						[[fallthrough]];

					case WA_ACTIVE:
						if (inst->autoResume && 
							(!inst->autoPause || inst->autoPauseExecuted) && 
							inst->IsMainMenuVisible()) // TODO: check if not in some submenu
						{
							inst->autoPauseExecuted = false;
							inst->SwitchMainMenu(false);
						}
						break;
				}
			}

			inst->WindowUpdateTitle();
			inst->MouseUpdate(true);
			return result;
		}

		// don't pause game on defocus
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			return DefWindowProc(wnd, msg, wParam, lParam);
		
		// restore proper handling of ShowCursor
		case WM_SETCURSOR:
			return DefWindowProc(wnd, msg, wParam, lParam);

		// do not send keyboard events to the inactive window
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			if (!HasFocus(wnd))
				return DefWindowProc(wnd, msg, wParam, lParam); // bypass the game
			
			break;
		}

		// handle the window menu Alt+Key hotkey messages
		case WM_SYSCOMMAND:
			if (wParam == SC_KEYMENU)
				return S_OK; // handled
			break;

		// do not send mouse events to the inactive window
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_MOUSEACTIVATE:
		case WM_MOUSEHOVER:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		{
			auto hasFocus = HasFocus(wnd);
			auto inClient = IsCursorInClientRect(wnd);

			if (!hasFocus && inClient)
			{
				SetCursor(LoadCursor(NULL,IDC_ARROW));
			}

			if (!hasFocus || !inClient)
			{
				return DefWindowProc(wnd, msg, wParam, lParam); // bypass the game
			}
			break;
		}

		case WM_STYLECHANGING:
		{
			auto styles = (STYLESTRUCT*)lParam;
			
			if (wParam == GWL_STYLE)
			{
				auto mask = WS_MINIMIZE | WS_MAXIMIZE;
				styles->styleNew &= mask;
				styles->styleNew |= inst->WindowStyle();
			}
			else if (wParam == GWL_EXSTYLE)
				styles->styleNew = inst->WindowStyleEx();
				
			break;
		}

		case WM_EXITSIZEMOVE:
			inst->WindowCalculateGeometry(true);

		// minimize, maximize, restore
		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED && wParam != SIZE_MAXHIDE) // prevent game from updating resolution for minimized window
				CallWindowProc(inst->oriWindowProc, wnd, msg, wParam, lParam); // inform the game
			return DefWindowProc(wnd, msg, wParam, lParam); // call default as otherwise maximization will not work correctly on later Windows versions

		// position or size changed
		case WM_WINDOWPOSCHANGED:
		{
			if (inst->windowUpdating || IsIconic(wnd)) break; // minimized

			bool updated = false;
			auto info = (WINDOWPOS*)lParam;

			// correct modern Windows styles invisible border
			RECT padding = inst->GetFrameSize(true);
			POINT pos = { info->x + padding.left, info->y + padding.top };
			if ((info->flags & SWP_NOMOVE) == 0)
			{
				if (pos.x != inst->windowPos.x || pos.y != inst->windowPos.y)
				{
					inst->windowPos = pos;
					updated = true;
				}
			}

			if ((info->flags & SWP_NOSIZE) == 0)
			{
				if (info->cx != inst->windowSize.x || info->cy != inst->windowSize.y)
				{
					inst->windowSize = { info->cx, info->cy };
					updated = true;
				}
			}

			if (updated)
			{
				inst->windowSizeClient = inst->ClientFromSize(inst->windowSize);
				inst->WindowCalculateGeometry();
				inst->WindowUpdateTitle();
				inst->SaveConfig();
			}

			break;
		}
	}

	return CallWindowProc(inst->oriWindowProc, wnd, msg, wParam, lParam);
}

POINT WindowedMode::SizeFromClient(POINT clientSize) const
{
	auto frame = GetFrameSize();
	clientSize.x += frame.left + frame.right;
	clientSize.y += frame.top + frame.bottom;
	return clientSize;
}

POINT WindowedMode::ClientFromSize(POINT windowSize) const
{
	auto frame = GetFrameSize();
	windowSize.x -= frame.left + frame.right;
	windowSize.y -= frame.top + frame.bottom;
	return windowSize;
}

RECT WindowedMode::GetFrameSize(bool padOnly) const
{
	RECT frame = { 0 };

	if (padOnly)
	{
		RECT base, extended;
		if (GetWindowRect(window, &base) &&
			SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &extended, sizeof(RECT))))
		{
			frame.left = extended.left - base.left;
			frame.top = extended.top - base.top;
			frame.right = base.right - extended.right;
			frame.bottom = base.bottom - extended.bottom;
		}
	}
	else
	{
		AdjustWindowRectEx(&frame, WindowStyle(), false, WindowStyleEx());
		frame.left *= -1; // offset to thickness
		frame.top *= -1; // offset to thickness
	}

	return frame;
}

HRESULT WindowedMode::D3dPresentHook(IDirect3DDevice8* self, const RECT* srcRect, const RECT* dstRect, HWND wnd, const RGNDATA* region)
{
	inst->MouseUpdate();

	if (inst->fpsCounter.update())
		inst->WindowUpdateTitle();

	auto result = inst->d3dPresentOri(self, srcRect, dstRect, wnd, region);

	// limit framerate in main menu
	if (inst->menuFrameRateLimit > 0 && inst->IsMainMenuVisible())
	{
		static DWORD prevTime = 0;
		DWORD currTime = timeGetTime();
	
		while (true)
		{
			DWORD delta = currTime - prevTime;

			if (delta >= (1000 / (DWORD)inst->menuFrameRateLimit))
				break;

			Sleep(1);
			currTime = timeGetTime();
		}
		prevTime = currTime;
	}

	return result;
}

HRESULT WindowedMode::D3dResetHook(IDirect3DDevice8* self, D3DPRESENT_PARAMETERS* parameters)
{
	// Always update geometry, ignore resolution changes from game
	inst->WindowCalculateGeometry();

	auto result = inst->d3dResetOri(self, (D3DPRESENT_PARAMETERS*)inst->d3dPresentParams9);

	if (SUCCEEDED(result))
		inst->UpdatePostEffect();

	return result;
}

bool WindowedMode::IsMainMenuVisible() const
{
	auto mgr = (CMenuManagerSA*)frontEndMenuManager;
	return mgr->m_bMenuActive;
}

void WindowedMode::SwitchMainMenu(bool show)
{
	auto mgr = (CMenuManagerSA*)frontEndMenuManager;
	if (show == mgr->m_bMenuActive) return; // already done

	mgr->m_bActivateMenuNextFrame = show;
	mgr->m_bDontDrawFrontEnd = !show;
}

void WindowedMode::MouseUpdate(bool force)
{
	auto hasFocus = HasFocus(window);

	POINT pos;
	GetCursorPos(&pos);

	RECT rect;
	GetClientRect(window, &rect);
	ClientToScreen(window, (LPPOINT)&rect.left);
	ClientToScreen(window, (LPPOINT)&rect.right);

	// cursor visibility
	bool inGame = hasFocus && PtInRect(&rect, pos);
	SetCursorVisible(!inGame);

	// keep cursor inside the window
	if (hasFocus || force)
	{
		if (!hasFocus || IsMainMenuVisible())
			ClipCursor(NULL);
		else
			ClipCursor(&rect);
	}
}

void WindowedMode::UpdatePostEffect()
{
	POINT oriSize;
	auto cam = *(RwCamera**)0xC1703C; // Scene.m_pRwCamera
	if (cam)
	{
		oriSize = { cam->frameBuffer->nWidth, cam->frameBuffer->nHeight }; // store
		cam->frameBuffer->nWidth = windowSizeClient.x;
		cam->frameBuffer->nHeight = windowSizeClient.y;
	}

	injector::cstd<void()>::call(0x7043D0); // CPostEffects::SetupBackBufferVertex()

	if (cam)
	{
		cam->frameBuffer->nWidth = oriSize.x; // restore
		cam->frameBuffer->nHeight = oriSize.y;
	}

	UpdateWidescreenFix();
}

void WindowedMode::UpdateWidescreenFix()
{
	static bool initialized = false;
	static HMODULE widescreenFix = NULL;
	static FARPROC updateFunc = NULL;

	if (!initialized)
	{
		widescreenFix = GetModuleHandle("GTASA.WidescreenFix.asi");

		if (widescreenFix)
			updateFunc = GetProcAddress(widescreenFix, "UpdateVars");

		initialized = true;
	}

	if (updateFunc)
		injector::stdcall<void()>::call(updateFunc);
}

