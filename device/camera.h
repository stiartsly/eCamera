#pragma once

#include <stdint.h>
#include <Windows.h>
#include <Control.h>
#include "obsolete.h"
#include "vsys.h"

#define DEFAULT_VIDEO_WIDTH     320
#define DEFAULT_VIDEO_HEIGHT    240

struct Camera;
struct CameraOps {
    int  (*prepare)     (struct Camera*);
    int  (*open)        (struct Camera*);
    int  (*startCapture)(struct Camera*);
    void (*stopCapture) (struct Camera*);
    void (*close)       (struct Camera*);
    void (*finalize)    (struct Camera*);
    int  (*prepare2)    (struct Camera*);
};

struct Camera {
    ISampleGrabberCB grabberCb;

    ICaptureGraphBuilder2* captureGraphBuilder2;
    IGraphBuilder*  graphBuilder;
    IMediaControl*  mediaControl;
    IMediaEvent*    mediaEvent;
    IVideoWindow*   videoWindow;
    IBaseFilter*    grabberFilter;
    ISampleGrabber* sampleGrabber;

    IMoniker*       videoMoniker;
    IBaseFilter*    captureFilter;

    struct vthread thread;
    struct vcond   cond;
    struct vlock   lock2;
    HWND hMainWnd;

    int32_t width;
    int32_t height;

    struct CameraOps* ops;

    void (*dispatchCb)(int32_t, int32_t, const uint8_t*, int32_t, void*);
    void* dispatchCtxt;
};

struct Camera* cameraCreate(void (*)(int32_t, int32_t, const uint8_t*, int32_t, void*), void*);
void cameraDestroy(struct Camera*);

void CameraSetDisptchFrameCb(struct Camera*, void(*)(void*, int, void*), void*);

