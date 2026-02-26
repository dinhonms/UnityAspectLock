// UnityAspectLock.cpp - Win32 DLL to lock Unity window aspect ratio (e.g., 16:9)
// Build: x64 (and optionally x86). Place DLL in Unity: Assets/Plugins/x86_64/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <cstdint>
#include <cmath>

#pragma comment(lib, "Comctl32.lib")

static HWND g_hwnd = nullptr;
static bool g_installed = false;
static float g_aspect = 9.0f / 16.0f;

static const UINT_PTR kSubclassId = 0xBADC0DE1;

static HWND FindMainWindowForCurrentProcess()
{
    struct Ctx { DWORD pid; HWND hwnd; };
    Ctx ctx{ GetCurrentProcessId(), nullptr };

    EnumWindows([](HWND hWnd, LPARAM lParam) -> BOOL {
        auto* c = reinterpret_cast<Ctx*>(lParam);

        DWORD pid = 0;
        GetWindowThreadProcessId(hWnd, &pid);
        if (pid != c->pid) return TRUE;

        // Must be a top-level visible "main" window
        if (!IsWindowVisible(hWnd)) return TRUE;
        if (GetWindow(hWnd, GW_OWNER) != nullptr) return TRUE;

        // Unity window class is commonly "UnityWndClass", but we don't depend on it.
        c->hwnd = hWnd;
        return FALSE; // stop
    }, reinterpret_cast<LPARAM>(&ctx));

    return ctx.hwnd;
}

static inline int RoundToInt(float v)
{
    return (int)std::lround(v);
}

static void ApplyAspect(RECT* r, WPARAM edge)
{
    if (!r || g_aspect <= 0.0f) return;

    int w = r->right - r->left;
    int h = r->bottom - r->top;
    if (w <= 0 || h <= 0) return;

    // Helpers based on anchoring the opposite side/corner
    auto setHeightFromWidth = [&](int newW) {
        int newH = RoundToInt((float)newW / g_aspect);
        return newH > 0 ? newH : h;
    };
    auto setWidthFromHeight = [&](int newH) {
        int newW = RoundToInt((float)newH * g_aspect);
        return newW > 0 ? newW : w;
    };

    switch (edge)
    {
        // Horizontal edge drag: user changes height -> derive width
        case WMSZ_TOP:
        case WMSZ_BOTTOM:
        {
            int newH = h;
            int newW = setWidthFromHeight(newH);

            // Keep left fixed, adjust right
            r->right = r->left + newW;
            break;
        }

        // Vertical edge drag: user changes width -> derive height
        case WMSZ_LEFT:
        case WMSZ_RIGHT:
        {
            int newW = w;
            int newH = setHeightFromWidth(newW);

            // Keep top fixed, adjust bottom
            r->bottom = r->top + newH;
            break;
        }

        // Corner drags: adjust one dimension to match the other depending on which changed more
        case WMSZ_TOPLEFT:
        case WMSZ_TOPRIGHT:
        case WMSZ_BOTTOMLEFT:
        case WMSZ_BOTTOMRIGHT:
        {
            // Decide whether width-driven or height-driven based on delta magnitude
            // (This feels natural while dragging corners.)
            float idealHFromW = (float)w / g_aspect;
            float idealWFromH = (float)h * g_aspect;

            float errH = std::fabs(idealHFromW - (float)h);
            float errW = std::fabs(idealWFromH - (float)w);

            bool driveByWidth = (errH < errW); // smaller error => closer intent

            if (driveByWidth)
            {
                int newH = setHeightFromWidth(w);

                if (edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT) {
                    // top moves, bottom anchored
                    r->top = r->bottom - newH;
                } else {
                    // bottom moves, top anchored
                    r->bottom = r->top + newH;
                }
            }
            else
            {
                int newW = setWidthFromHeight(h);

                if (edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT) {
                    // left moves, right anchored
                    r->left = r->right - newW;
                } else {
                    // right moves, left anchored
                    r->right = r->left + newW;
                }
            }
            break;
        }

        default:
            break;
    }
}

static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    if (uIdSubclass != kSubclassId)
        return DefSubclassProc(hWnd, msg, wParam, lParam);

    if (msg == WM_SIZING)
    {
        RECT* r = reinterpret_cast<RECT*>(lParam);
        ApplyAspect(r, wParam);
        return TRUE; // we modified the RECT
    }

    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

extern "C"
{
    __declspec(dllexport) int UnityAspectLock_Install(float aspectWidth, float aspectHeight)
    {
        if (g_installed) return 1;

        if (aspectWidth <= 0.0f || aspectHeight <= 0.0f) return 0;
        g_aspect = aspectWidth / aspectHeight;

        g_hwnd = FindMainWindowForCurrentProcess();
        if (!g_hwnd) return 0;

        // Ensure common controls loaded (for SetWindowSubclass)
        INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
        InitCommonControlsEx(&icc);

        if (!SetWindowSubclass(g_hwnd, SubclassProc, kSubclassId, 0))
            return 0;

        g_installed = true;
        return 1;
    }

    __declspec(dllexport) void UnityAspectLock_Uninstall()
    {
        if (!g_installed) return;
        if (g_hwnd)
            RemoveWindowSubclass(g_hwnd, SubclassProc, kSubclassId);

        g_hwnd = nullptr;
        g_installed = false;
    }

    __declspec(dllexport) int UnityAspectLock_IsInstalled()
    {
        return g_installed ? 1 : 0;
    }
}