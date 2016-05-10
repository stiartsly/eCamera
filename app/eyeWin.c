#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "eyeWin.h"
#include "vlog.h"
#include "vsys.h"

static
int32_t _eyeWinOpsStart(struct EyeWin* win)
{
    if (!win) return -1;

    //todo;
    return 0;
}

static
void _eyeWinOpsStop(struct EyeWin* win)
{
    if (!win) return;

    //todo;
    return;
}

long __stdcall WindowProcedure(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_DESTROY:

        printf("destroy windows\n");
        PostQuitMessage(0);;
        return 0L;
    case WM_LBUTTONDOWN:
        // fall thru
    default:
       // printf(".");
        return DefWindowProc(window, msg, wp, lp);
    }
}

static
int _windowEntry(void* args)
{
    struct EyeWin* win = (struct EyeWin*)args;

    RECT r = {
        .left = 0,
        .top  = 0,
        .right = win->width,
        .bottom = win->height,
    };

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
    int32_t w = r.right - r.left;
    int32_t h = r.bottom - r.top;

    if (w > GetSystemMetrics(SM_CXSCREEN)) {
        w = GetSystemMetrics(SM_CXSCREEN);
    }
    if (h > GetSystemMetrics(SM_CYSCREEN)) {
        h = GetSystemMetrics(SM_CYSCREEN);
    }

    printf("<%s> w:%d, h:%d\n", __FUNCTION__, w, h);

    WNDCLASSEX wndclass = {
        sizeof(WNDCLASSEX),
        CS_DBLCLKS,
        WindowProcedure,
        0,
        0,
        GetModuleHandle(0),
        LoadIcon(0,IDI_APPLICATION),
        LoadCursor(0,IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        0,
        L"myclass",
        LoadIcon(0,IDI_APPLICATION)
    };
    if (RegisterClassEx(&wndclass)) {
        win->hwnd = CreateWindowW(L"myclass", L"eyeWin", WS_OVERLAPPEDWINDOW | WS_CAPTION |WS_CLIPCHILDREN, 800, 50, w, h, NULL, NULL, win->hInst, NULL);
        if (!win->hwnd) {
            vlogE("Create Window failed (err:%d)", GetLastError());
            return -1;
        }
        ShowWindow(win->hwnd, SW_SHOWDEFAULT);
        vlock_enter(&win->lock);
        vcond_signal(&win->cond);
        vlock_leave(&win->lock);

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0)) DispatchMessage(&msg);
    } else {
        vlogE("Register class failed (%d)\n", GetLastError());
        vlock_enter(&win->lock);
        vcond_signal(&win->cond);
        vlock_leave(&win->lock);
    }

    return 0;
}


static
void _eyeWinOpsRenderFrame(struct EyeWin* win, int32_t width, int32_t height, const uint8_t* frame)
{
    if (!win) return ;
    if (width <= 0) return;
    if (height <= 0) return;
    if (!frame) return;

    vlogI("Render to window");

    if (!win->hwnd) {
        win->width = width;
        win->height = height;
        printf("<%s> width:%d, height:%d\n", __FUNCTION__, win->width, win->height);
        vcond_init(&win->cond);
        vlock_init(&win->lock);
        vthread_init(&win->thread,  _windowEntry, win);
        vthread_start(&win->thread);
        vlock_enter(&win->lock);
        vcond_wait(&win->cond, &win->lock);
        vlock_leave(&win->lock);
    } else {
        BITMAPINFO bmi = {
           .bmiHeader = {
               .biSize = sizeof(BITMAPINFOHEADER),
               .biWidth = width,
               .biHeight = -height,
               .biPlanes = 1,
               .biBitCount = 32,
               .biCompression = BI_RGB,
           }
        };
        RECT r = {0};
        GetClientRect(win->hwnd, &r);
        HDC dc = GetDC(win->hwnd);

        if(width == r.right && height == r.bottom) {
            SetDIBitsToDevice(dc, 0, 0, width, height, 0, 0, 0, height, frame, &bmi, DIB_RGB_COLORS);
        } else {
            StretchDIBits(dc, 0, 0, r.right, r.bottom, 0, 0, width, height, frame, &bmi, DIB_RGB_COLORS, SRCCOPY);
        }
    }

#if 0
    if (!win->hwnd) {
        RECT r = {
             .left = 0,
             .top  = 0,
             .right = width,
             .bottom = height
       };

       AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);
       int32_t w = r.right - r.left;
       int32_t h = r.bottom - r.top;

       if (w > GetSystemMetrics(SM_CXSCREEN)) {
           w = GetSystemMetrics(SM_CXSCREEN);
       }
       if (h > GetSystemMetrics(SM_CYSCREEN)) {
           h = GetSystemMetrics(SM_CYSCREEN);
       }

       //HINSTANCE hinst = AfxGetInstanceHandle();(NULL);
       //printf("hmodule:0x%x\n", hinst);

       WNDCLASSEX wndclass = {
           sizeof(WNDCLASSEX),
           CS_DBLCLKS,
           WindowProcedure,
           0,
           0,
           GetModuleHandle(0),
           LoadIcon(0,IDI_APPLICATION),
           LoadCursor(0,IDC_ARROW),
           (HBRUSH)(COLOR_WINDOW + 1),
           0,
           L"myclass",
           LoadIcon(0,IDI_APPLICATION)
       };
       if (RegisterClassEx(&wndclass)) {
           win->hwnd = CreateWindowW(L"myclass", L"eyeWin", WS_OVERLAPPEDWINDOW, 300, 500, w, h, NULL, NULL, win->hInst, NULL);
           if (!win->hwnd) {
               vlogE("Create Window failed (err:%d)", GetLastError());
               exit(-1);
               return;
           }
           ShowWindow(win->hwnd, SW_SHOWDEFAULT);

         //  MSG msg;
          //  while (GetMessage(&msg, 0, 0, 0)) DispatchMessage(&msg);
       } else {
           vlogE("Register class failed (%d)\n", GetLastError());
       }
    } else {
       BITMAPINFO bmi = {
            .bmiHeader = {
                .biSize = sizeof(BITMAPINFOHEADER),
                .biWidth = width,
                .biHeight = -height,
                .biPlanes = 1,
                .biBitCount = 32,
                .biCompression = BI_RGB,
            }
        };


        RECT r = {0};
        GetClientRect(win->hwnd, &r);
        HDC dc = GetDC(win->hwnd);

        if(width == r.right && height == r.bottom) {
            SetDIBitsToDevice(dc, 0, 0, width, height, 0, 0, 0, height, frame, &bmi, DIB_RGB_COLORS);
        } else {
            StretchDIBits(dc, 0, 0, r.right, r.bottom, 0, 0, width, height, frame, &bmi, DIB_RGB_COLORS, SRCCOPY);
        }
    }
#endif
    return;
}

static
struct EyeWinOps eyeWinOps = {
    .start       = _eyeWinOpsStart,
    .stop        = _eyeWinOpsStop,
    .renderFrame = _eyeWinOpsRenderFrame
};


struct EyeWin* eyeWinCreate(void)
{
    struct EyeWin* w = calloc(1, sizeof(*w));
    if (!w) {
        vlogE("calloc problem");
        return NULL;
    }

    w->ops  = &eyeWinOps;
    w->hwnd = NULL;
    w->width = 0;
    w->height = 0;
    w->hInst = GetModuleHandle(NULL);
    //todo;
    return w;
}

void eyeWinDestroy(struct EyeWin* win)
{
    if (!win) return;

    //todo;
}

