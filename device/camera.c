#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <Control.h>
#include <dshow.h>
#include <objbase.h>
#include <uuids.h>
#include <strmif.h>
#include <Windows.h>
#include "obsolete.h"
#include "camera.h"
#include "vlog.h"
#include "vsys.h"

#define SAFE_RELEASE(ppt) do { \
        if (ppt) ppt->lpVtbl->Release(ppt); \
        ppt = NULL; \
    }while(0)

extern ISampleGrabberCB grabberCb;

IMediaEvent* g_videoEvent = NULL;
IVideoWindow* g_videoWindow = NULL;


static
int _cameraOps_prepare(struct Camera* camera)
{
    ICaptureGraphBuilder2* captureGraphBuilder2 = NULL;
    IGraphBuilder*   graphBuilder = NULL;
    IMediaControl*   mediaControl = NULL;
    IMediaEvent*     mediaEvent   = NULL;
    IVideoWindow*    videoWindow  = NULL;
    IBaseFilter*    grabberFilter = NULL;
    ISampleGrabber* sampleGrabber = NULL;
    HRESULT hr = 0;

    assert(camera);

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC, &IID_IGraphBuilder, (void**)&graphBuilder);
    if (FAILED(hr)) {
        vlogE("CoCreateInstance for CLSID_FilterGraph failed");
        goto errorExit;
    }
    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, &IID_ICaptureGraphBuilder2, (void**)&captureGraphBuilder2);
    if (FAILED(hr)) {
        vlogE("CoCreateInstance for CLSID_CaptureGraphBuilder2 failed.");
        goto errorExit;
    }
    hr = graphBuilder->lpVtbl->QueryInterface(graphBuilder, &IID_IMediaControl, (void**)&mediaControl);
    if (FAILED(hr)) {
        vlogE("QueryInterface for IID_IMediaControl failed");
        goto errorExit;
    }
    hr = graphBuilder->lpVtbl->QueryInterface(graphBuilder, &IID_IMediaEvent, (void**)&mediaEvent);
    if (FAILED(hr)) {
        vlogE("QueryInterface for IID_IMediaEvent failed");
        goto errorExit;
    }

#if 1
    hr = graphBuilder->lpVtbl->QueryInterface(graphBuilder, &IID_IVideoWindow, (void**)&videoWindow);
    if (FAILED(hr)) {
        vlogE("QueryInterface for IID_IVideoWindow failed");
        goto errorExit;
    }
#endif

    hr = CoCreateInstance(&CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void**)&grabberFilter);
    if (FAILED(hr)) {
        goto errorExit;
    }
    hr = grabberFilter->lpVtbl->QueryInterface(grabberFilter, &IID_ISampleGrabber, (void**)&sampleGrabber);
    if (FAILED(hr)) {
        goto errorExit;
    }

    hr = captureGraphBuilder2->lpVtbl->SetFiltergraph(captureGraphBuilder2, graphBuilder);
    if (FAILED(hr)) {
        vlogE("Couldn't set filter graph");
        goto errorExit;
    }

    g_videoWindow = videoWindow;
    g_videoEvent  = mediaEvent;



    camera->captureGraphBuilder2 = captureGraphBuilder2;
    camera->graphBuilder  = graphBuilder;
    camera->mediaControl  = mediaControl;
    camera->mediaEvent    = mediaEvent;
    camera->videoWindow   = videoWindow;
    camera->grabberFilter = grabberFilter;
    camera->sampleGrabber = sampleGrabber;
    return 0;

errorExit:
    SAFE_RELEASE(grabberFilter);
    SAFE_RELEASE(videoWindow);
    SAFE_RELEASE(mediaEvent);
    SAFE_RELEASE(mediaControl);
    SAFE_RELEASE(graphBuilder);
    SAFE_RELEASE(captureGraphBuilder2);
    return -1;
}

#if 1
HRESULT HandleGraphEvent(void)
{
    LONG evCode;
    LONG_PTR param1, param2;
    HRESULT hr=S_OK;


    if (!g_videoEvent) return E_POINTER;


    while(SUCCEEDED(g_videoEvent->lpVtbl->GetEvent(g_videoEvent, &evCode, &param1, &param2, 0)))
    {
        //
        // Free event parameters to prevent memory leaks associated with
        // event parameter data.  While this application is not interested
        // in the received events, applications should always process them.
        //
        hr = g_videoEvent->lpVtbl->FreeEventParams(g_videoEvent, evCode, param1, param2);

        // Insert event processing code here, if desired
    }
    return hr;
}

#define WM_GRAPHNOTIFY 555

long __stdcall manWndMsgProc(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_GRAPHNOTIFY:
            HandleGraphEvent();
            break;


        case WM_SIZE:
         //   ResizeVideoWindow();
            break;


        case WM_WINDOWPOSCHANGED:
       //     ChangePreviewState(! (IsIconic(hwnd)));
            break;


        case WM_CLOSE:
            // Hide the main window while the graph is destroyed
          //  ShowWindow(ghApp, SW_HIDE);
         //   CloseInterfaces();  // Stop capturing and release interfaces
            break;


        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

    }
    // Pass this message to the video window for notification of system changes
    if (g_videoWindow) {
        g_videoWindow->lpVtbl->NotifyOwnerMessage(g_videoWindow,(LONG_PTR)window, msg, wp, lp);
    }

    return DefWindowProc(window, msg, wp, lp);
}

static
int _mainWindowEntry(void* args)
{
    struct Camera* cam = (struct Camera*)args;

    RECT r = {
        .left = 0,
        .top  = 0,
        .right = cam->width,
        .bottom = cam->height,
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
        manWndMsgProc,
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
        cam->hMainWnd = CreateWindowW(L"myclass", L"camWin", WS_OVERLAPPEDWINDOW|WS_CAPTION | WS_CLIPCHILDREN, 50, 50, w, h, NULL, NULL, GetModuleHandle(0), NULL);
        if (!cam->hMainWnd) {
            vlogE("Create Window failed (err:%d)", GetLastError());
            return -1;
        }
        ShowWindow(cam->hMainWnd, SW_SHOWDEFAULT);
        vlock_enter(&cam->lock2);
        vcond_signal(&cam->cond);
        vlock_leave(&cam->lock2);

        MSG msg;
        while (GetMessage(&msg, 0, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        vlogE("Register class failed (%d)\n", GetLastError());
        vlock_enter(&cam->lock2);
        vcond_signal(&cam->cond);
        vlock_leave(&cam->lock2);
    }

    return 0;
}
#endif

static
int _cameraOps_prepare2(struct Camera* camera)
{
    if (!camera) return -1;

    camera->width = 640;
    camera->height = 480;
    vcond_init(&camera->cond);
    vlock_init(&camera->lock2);
    vthread_init(&camera->thread, _mainWindowEntry, camera);
    vthread_start(&camera->thread);
    vlock_enter(&camera->lock2);
    vcond_wait(&camera->cond, &camera->lock2);
    vlock_leave(&camera->lock2);

    return 0;

}

static
int _cameraOps_open(struct Camera* camera)
{
    ICreateDevEnum* devEnum = NULL;
    IEnumMoniker*   monEnum = NULL;
    IMoniker*       moniker = NULL;
    IBaseFilter*    captureFilter = NULL;
    HRESULT hr = 0;

    if (!camera) return -1;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void**)&devEnum);
    if (FAILED(hr)) {
        vlogE("CoCreateInstance for CLSID_SystemDeviceNum failed");
        goto errorExit;
    }
    hr = devEnum->lpVtbl->CreateClassEnumerator(devEnum, &CLSID_VideoInputDeviceCategory, &monEnum, 0);
    if (S_OK != hr) {
        vlogE("CreateClassEnumerator for VideoInputDeviceCategory.");
        goto errorExit;
    }

    ULONG fetched = 0;
    hr = monEnum->lpVtbl->Next(monEnum, 1, &moniker, &fetched);
    if (S_OK != hr) {
        vlogE("Enumerate moniker failed");
        goto errorExit;
    }
    hr = moniker->lpVtbl->BindToObject(moniker, NULL, NULL, &IID_IBaseFilter, &captureFilter);
    if (FAILED(hr)) {
        vlogE("BindToObejct for video failed");
        goto errorExit;
    }

    SAFE_RELEASE(monEnum);
    SAFE_RELEASE(devEnum);

    camera->videoMoniker  = moniker;
    camera->captureFilter = captureFilter;

    return 0;

errorExit:
    SAFE_RELEASE(moniker);
    SAFE_RELEASE(monEnum);
    SAFE_RELEASE(devEnum);
    return -1;
}

static
int _cameraOps_startCapture(struct Camera* camera)
{
    ICaptureGraphBuilder2* captureGraphBuilder2 = camera->captureGraphBuilder2;
    IGraphBuilder*  graphBuilder  = camera->graphBuilder;
    IBaseFilter*    captureFilter = camera->captureFilter;
    IBaseFilter*    grabberFilter = camera->grabberFilter;
    ISampleGrabber* sampleGrabber = camera->sampleGrabber;
    HRESULT hr = 0;

    assert(graphBuilder);
    assert(captureFilter);
    assert(grabberFilter);
    assert(sampleGrabber);

    hr = graphBuilder->lpVtbl->AddFilter(graphBuilder, captureFilter, L"Video Capture");
    if (FAILED(hr)) {
        vlogE("Could not add video capture filter to Graph");
        return -1;
    }
    hr = graphBuilder->lpVtbl->AddFilter(graphBuilder, grabberFilter, L"Sample Grabber");
    if (FAILED(hr)){
        vlogE("Could not add grabber filer to Graph");
        return -1;
    }

    AM_MEDIA_TYPE mt;
    memset(&mt, 0, sizeof(mt));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24;

    sampleGrabber->lpVtbl->SetMediaType(sampleGrabber, &mt);
    sampleGrabber->lpVtbl->SetBufferSamples(sampleGrabber, TRUE);

    hr = captureGraphBuilder2->lpVtbl->RenderStream(captureGraphBuilder2, &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, captureFilter, grabberFilter, NULL);
    if (FAILED(hr)) {
        vlogE("Could not render video capture stream");
        return -1;
    }

    sampleGrabber->lpVtbl->GetConnectedMediaType(sampleGrabber, &mt);
    VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*) mt.pbFormat;

    camera->width  = vih->bmiHeader.biWidth;
    camera->height = vih->bmiHeader.biHeight;
    vlogI("<%s> camera->widht:%d, camera->height:%d\n", __FUNCTION__, camera->width, camera->height);

    hr = sampleGrabber->lpVtbl->SetCallback(sampleGrabber, &camera->grabberCb, 0);
    if (FAILED(hr)) {
        vlogE("Couldn't set grabber filter's callback");
        return -1;
    }

    if (camera->videoWindow) {
#if 1
        camera->videoWindow->lpVtbl->put_Owner(camera->videoWindow, (OAHWND)camera->hMainWnd);
        camera->videoWindow->lpVtbl->put_WindowStyle(camera->videoWindow, WS_CHILD | WS_CLIPCHILDREN);
        IMediaEventEx* videoEventEx = (IMediaEventEx*)g_videoEvent;
        hr = videoEventEx->lpVtbl->SetNotifyWindow(videoEventEx, (OAHWND)camera->hMainWnd, WM_GRAPHNOTIFY, 0);
        if (hr != S_OK) {
            vlogE("setNotifyWindow faile");

        } else {
            vlogI("setNotifyWindow succeeed (0x%x)", camera->hMainWnd);
        }
#endif
        camera->videoWindow->lpVtbl->put_Left(camera->videoWindow, 0);
        camera->videoWindow->lpVtbl->put_Width(camera->videoWindow,camera->width);
        camera->videoWindow->lpVtbl->put_Top(camera->videoWindow, 0);
        camera->videoWindow->lpVtbl->put_Height(camera->videoWindow, camera->height);
        //camera->videoWindow->lpVtbl->put_AutoShow(camera->videoWindow,OATRUE);
        camera->videoWindow->lpVtbl->put_Visible(camera->videoWindow, OATRUE);
        camera->videoWindow->lpVtbl->put_Caption(camera->videoWindow, L"eCamera");

        vlogI("1111<%s> width:%d, height:%d\n", __FUNCTION__, camera->width, camera->height);
    }

    hr = camera->mediaControl->lpVtbl->Run(camera->mediaControl);
    if (FAILED(hr)) {
        vlogE("Couldn't run the graph.");
        return -1;
    }
    return 0;
}

static
void _cameraOps_stopCapture(struct Camera* camera)
{
    assert(camera);

    if (camera->mediaControl) {
        camera->mediaControl->lpVtbl->Stop(camera->mediaControl);
        camera->mediaControl->lpVtbl->StopWhenReady(camera->mediaControl);
    }

    camera->videoWindow->lpVtbl->put_Visible(camera->videoWindow, OAFALSE);

    return;
}

static
void _cameraOps_close(struct Camera* camera)
{
    SAFE_RELEASE(camera->videoMoniker);
    SAFE_RELEASE(camera->captureFilter);
    return;
}

static
void _cameraOps_finalize(struct Camera* camera)
{
    assert(camera);

    if (camera->mediaControl) {
        camera->mediaControl->lpVtbl->StopWhenReady(camera->mediaControl);
    }
    if (camera->videoWindow) {
        camera->videoWindow->lpVtbl->put_Visible(camera->videoWindow, OAFALSE);
    }
    SAFE_RELEASE(camera->videoWindow);
    SAFE_RELEASE(camera->mediaControl);
    SAFE_RELEASE(camera->mediaEvent);
    SAFE_RELEASE(camera->captureFilter);
    SAFE_RELEASE(camera->grabberFilter);
    SAFE_RELEASE(camera->graphBuilder);
    SAFE_RELEASE(camera->captureGraphBuilder2);

    return;
}

static
struct CameraOps cameraOps = {
    .prepare      = _cameraOps_prepare,
    .open         = _cameraOps_open,
    .startCapture = _cameraOps_startCapture,
    .stopCapture  = _cameraOps_stopCapture,
    .close        = _cameraOps_close,
    .finalize     = _cameraOps_finalize,

    .prepare2     = _cameraOps_prepare2,
};

struct Camera* cameraCreate(void (*dispatchCb)(int32_t, int32_t, const uint8_t*, int32_t, void*), void* userData)
{
    struct Camera* cam = NULL;
    HRESULT hr = 0;

    if (!dispatchCb) return NULL;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        vlogE("CoInitialize failed.");
        return NULL;
    }

    cam = (struct Camera*)calloc(1, sizeof(*cam));
    if (!cam) {
        vlogE("Calloc [Camera] failed");
        CoUninitialize();
        return NULL;
    }

    cam->ops = &cameraOps;
    cam->grabberCb  = grabberCb;
    cam->dispatchCb = dispatchCb;
    cam->dispatchCtxt = userData;

    int ret = cam->ops->prepare(cam);
    if (ret < 0) {
        vlogE("Prepare [Camera] failed");
        free(cam);
        CoUninitialize();
        return NULL;
    }
    ret = cam->ops->open(cam);
    if (ret < 0) {
        vlogE("Open [Camera] failed");
        cam->ops->finalize(cam);
        free(cam);
        CoUninitialize();
        return NULL;
    }

    ret = cam->ops->prepare2(cam);
    if (ret < 0) {
        vlogE("Prepare step 2 (camera window) failed");
        cam->ops->close(cam);
        cam->ops->finalize(cam);
        free(cam);
        CoUninitialize();
        return NULL;
    }

    return cam;
}

void cameraDestroy(struct Camera* cam)
{
    if (!cam) {
        return ;
    }

    cam->ops->close(cam);
    cam->ops->finalize(cam);

    free(cam);
    CoUninitialize();

    return;
}

