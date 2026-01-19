#pragma once
#include <stdint.h>
#include "d3d8/d3d8.h"
#include "d3d8/dinput.h"
#include "IniReader.h"
#include "injector/injector.hpp"
#include "injector/assembly.hpp"
#include "injector/calling.hpp"

class FpsCounter
{
protected:
	unsigned int fps;
	unsigned int count;
	DWORD prevTime;

public:
	FpsCounter() : fps(30), count(0), prevTime(timeGetTime() / 1000)
	{
	}

	bool update()
	{
		auto currTime = timeGetTime() / 1000;
		if (prevTime != currTime)
		{
			fps = count;
			prevTime = currTime;
			count = 1; // this frame is in next period already
		}
		else
			count++;

		static DWORD prevFps = -1;
		auto updated = fps != prevFps;
		prevFps = fps;

		return updated;
	}

	unsigned int get() const
	{
		return fps;
	}
};

typedef struct _D3DPRESENT_PARAMETERS_D3D9_
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
} D3DPRESENT_PARAMETERS_D3D9;

struct RwVideoMode
{
	int32_t width;
	int32_t height;
	int32_t depth;
	int32_t flags;
};

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
	//RsInputDevice keyboard;
	//RsInputDevice mouse;
	//RsInputDevice pad;
};

enum RwRasterType
{
	rwRASTERTYPENORMAL = 0x00,
	rwRASTERTYPEZBUFFER = 0x01,
	rwRASTERTYPECAMERA = 0x02,
	rwRASTERTYPETEXTURE = 0x04,
	rwRASTERTYPECAMERATEXTURE = 0x05,
	rwRASTERTYPEMASK = 0x07,
	rwRASTERPALETTEVOLATILE = 0x40,
	rwRASTERDONTALLOCATE = 0x80,
	rwRASTERTYPEFORCEENUMSIZEINT = ((int32_t)((~((uint32_t)0)) >> 1))
};

struct IDirectInputDeviceA;
struct IDirectInputA;
struct IDirect3DSwapChain8;

struct GameDxInput
{
	unsigned char __pad00[24];
	IDirectInputA* pInput;
	IDirectInputDeviceA* pInputDevice;
	unsigned char __pad04[8];
};

typedef float RwReal;
struct RwV2d
{
	RwReal x;
	RwReal y;
};

struct RwV3d
{
	RwReal x;
	RwReal y;
	RwReal z;
};

struct RwPlane
{
	RwV3d normal;
	RwReal distance;
};

struct RwFrustumPlane
{
	RwPlane plane;
	uint8_t closestX;
	uint8_t closestY;
	uint8_t closestZ;
	uint8_t pad;
};

struct RwBBox
{
	RwV3d sup;
	RwV3d inf;
};

struct RwRaster
{
	RwRaster* pParent;
	uint8_t* pPixels;
	uint8_t* pPalette;
	int32_t nWidth;
	int32_t nHeight;
	int32_t nDepth;
	int32_t nStride;
	int16_t nOffsetX;
	int16_t nOffsetY;
	uint8_t cType;
	uint8_t cFlags;
	uint8_t cPrivateFlags;
	uint8_t cFormat;
	uint8_t* pOriginalPixels;
	int32_t nOriginalWidth;
	int32_t nOriginalHeight;
	int32_t nOriginalStride;
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
	RwV2d viewWindow;
	RwV2d recipViewWindow;
	RwV2d viewOffset;
	RwReal nearPlane;
	RwReal farPlane;
	RwReal fogPlane;
	RwReal zScale, zShift;
	RwFrustumPlane frustumPlanes[6];
	RwBBox frustumBoundBox;
	RwV3d frustumCorners[8];
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
	//D3DDISPLAYMODE mode; expanded below
	UINT width;
	UINT height;
	UINT refreshRate;
	D3DFORMAT format;

	DWORD flags;
};

struct CMenuManagerSA
{
	int8_t m_nStatsScrollDirection;
	float m_fStatsScrollSpeed;
	uint8_t m_nSelectedRow;
	char field_9[23];
	bool m_PrefsUseVibration;
	bool m_bHudOn;
	char field_22[2];
	int32_t m_nRadarMode;
	char field_28[4];
	int32_t m_nTargetBlipIndex;
	int8_t m_nSysMenu;
	bool m_DisplayControllerOnFoot;
	bool m_bDontDrawFrontEnd;
	bool m_bActivateMenuNextFrame;
	bool m_bMenuAccessWidescreen;
	char field_35;
	char field_36[2];
	int32_t m_KeyPressedCode;
	int32_t m_PrefsBrightness;
	float m_fDrawDistance;
	bool m_bShowSubtitles;
	bool m_abPrefsMapBlips[5];
	bool m_bMapLegend;
	bool m_bWidescreenOn;
	bool m_bPrefsFrameLimiter;
	bool m_bRadioAutoSelect;
	char field_4E;
	int8_t m_nSfxVolume;
	int8_t m_nRadioVolume;
	bool m_bRadioEq;
	int8_t m_nRadioStation;
	char field_53;
	int32_t m_nCurrentScreenItem;
	bool m_bQuitGameNoDVD;
	bool m_bDrawingMap;
	bool m_bStreamingDisabled;
	bool m_bAllStreamingStuffLoaded;
	bool m_bMenuActive;
	bool m_bStartGameLoading;
	int8_t m_nGameState;
	bool m_bIsSaveDone;
	bool m_bLoadingData;
	float m_fMapZoom;
	RwV2d m_vMapOrigin;
	RwV2d m_vMousePos;
	bool m_bMapLoaded;
	int32_t m_nTitleLanguage;
	int32_t m_nTextLanguage;
	uint8_t m_nPrefsLanguage;
	uint8_t m_nPreviousLanguage;
	int32_t m_SystemLanguage;
	bool field_8C;
	int32_t m_ListSelection;
	int32_t field_94;
	uint8_t* m_GalleryImgBuffer;
	char field_9C[16];
	int32_t m_nUserTrackIndex;
	int8_t m_nRadioMode;
	bool m_bInvertPadX1;
	bool m_bInvertPadY1;
	bool m_bInvertPadX2;
	bool m_bInvertPadY2;
	bool m_bSwapPadAxis1;
	bool m_bSwapPadAxis2;
	bool m_RedefiningControls;
	bool m_DisplayTheMouse;
	int32_t m_nMousePosX;
	int32_t m_nMousePosY;
	bool m_bPrefsMipMapping;
	bool m_bTracksAutoScan;
	int32_t m_nPrefsAntialiasing;
	int32_t m_nDisplayAntialiasing;
	int8_t m_ControlMethod;
	int32_t m_nPrefsVideoMode;
	int32_t m_nDisplayVideoMode;
	int32_t m_nCurrentRwSubsystem;
	int32_t m_nMousePosWinX;
	int32_t m_nMousePosWinY;
	bool m_bSavePhotos;
	bool m_bMainMenuSwitch;
	// incomplete
};

static inline DWORD Crc32(const BYTE* data, size_t size)
{
	DWORD crc = ~0;
	for (size_t i = 0; i < size; i++)
	{
		crc = crc ^ data[i];
		for (size_t j = 0; j < 8; j++)
		{
			DWORD mask = ~(crc & 1) + 1;
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}
	return ~crc;
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
	ClientToScreen(wnd, (LPPOINT)&rect.left); // left& top
	ClientToScreen(wnd, (LPPOINT)&rect.right); // right & bottom

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
	int tries = 128; // infinite loop prevention
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
	auto hash = Crc32(buffer.data(), size);

	if (hash != expectedHash)
	{
		ShowError("Memory \"%s\" modified!\nExpected hash: 0x%08X\nActual hash: 0x%08X", name, expectedHash, hash);
	}
}

