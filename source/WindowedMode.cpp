// Minimal borderless fullscreen implementation
#include "WindowedMode.h"
#include "Windowed_GtaSA.h"
#include <cstdio>

WindowedMode::WindowedMode(
    uintptr_t gameState,
    uintptr_t rsGlobal,
    uintptr_t d3dDevice,
    uintptr_t d3dPresentParams,
    uintptr_t rwVideoModes,
    uintptr_t RwEngineGetNumVideoModes,
    uintptr_t RwEngineGetCurrentVideoMode
) :
    gameState(*(GameState*)gameState),
    rsGlobalSA((RsGlobalTypeSA*)rsGlobal),
    d3dDevice(*(IDirect3DDevice8**)d3dDevice),
    d3dPresentParams((D3DPRESENT_PARAMETERS*)d3dPresentParams),
    rwVideoModes((DisplayMode**)rwVideoModes),
    RwEngineGetNumVideoModes(*(DWORD(*)())RwEngineGetNumVideoModes),
    RwEngineGetCurrentVideoMode(*(DWORD(*)())RwEngineGetCurrentVideoMode)
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

    inst->WindowCalculateGeometry();
    sprintf_s(inst->windowTitle, "%s", inst->rsGlobalSA->AppName);

    WNDCLASSA wndClass = {};
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = inst->windowClassName;
    wndClass.hIcon = inst->windowIcon;
    wndClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpfnWndProc = &WindowedMode::WindowProc;

    UnregisterClass(inst->windowClassName, hInstance);
    RegisterClass(&wndClass);

    inst->window = CreateWindowEx(
        inst->WindowStyleEx(),
        inst->windowClassName,
        inst->windowTitle,
        inst->WindowStyle(),
        inst->windowPos.x, inst->windowPos.y,
        inst->windowSize.x, inst->windowSize.y,
        NULL,
        NULL,
        hInstance,
        0);

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
    inst->WindowCalculateGeometry();
    return inst->d3dResetOri(self, (D3DPRESENT_PARAMETERS*)inst->d3dPresentParams);
}
