#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "obsolete.h"
#include "camera.h"

static
STDMETHODIMP _grabberCbVtbl_QueryInterface(ISampleGrabberCB *cb, REFIID riid, LPVOID FAR * lppvObj)
{
    //DO NOTHING;
    return 0;
}

static
STDMETHODIMP_(ULONG) _grabberCbVtbl_AddRef(ISampleGrabberCB *cb)
{
    //DO NOTHING;
    return 1;
}

static
STDMETHODIMP_(ULONG) _grabberCbVtbl_Release(ISampleGrabberCB *cb)
{
    //DO NOTHING;
    return 0;
}

static
HRESULT STDMETHODCALLTYPE _grabberCbVtbl_SampleCB(ISampleGrabberCB *cb, double sampleTime, IMediaSample *sample)
{
    void* buffer = NULL;
    int32_t length = 0;

    sample->lpVtbl->GetPointer(sample, (BYTE**)&buffer);
    length = sample->lpVtbl->GetActualDataLength(sample);

    struct Camera* c = (struct Camera*)cb;
    if (length != c->width * c->height * 3) {
        return -1;
    }

    c->dispatchCb(c->width, c->height, buffer, length, c->dispatchCtxt);
    return S_OK;
}

static
HRESULT STDMETHODCALLTYPE _grabberCbVtbl_BufferCB(ISampleGrabberCB* cb, double sampleTime, BYTE * buffer, long bufferSize )
{
    struct Camera* camera = (struct Camera*)cb;
    BITMAPINFOHEADER bih;
    static int times = 0;

#if 1
    memset(&bih, 0, sizeof(bih));
    bih.biSize = sizeof( bih );
    bih.biWidth  = camera->width;
    bih.biHeight = camera->height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;

    BITMAPINFO bi;
    bi.bmiHeader = bih;

    printf("grabber: bufferSize:%d\n", bufferSize);
    if (camera->dispatchCb) {
        camera->dispatchCb(bih.biWidth, bih.biHeight, buffer, bufferSize, camera->dispatchCtxt);
    }
#endif
    return S_OK;
}

static
ISampleGrabberCBVtbl grabberVtbl = {
    .QueryInterface = _grabberCbVtbl_QueryInterface,
    .AddRef         = _grabberCbVtbl_AddRef,
    .Release        = _grabberCbVtbl_Release,
    .SampleCB       = _grabberCbVtbl_SampleCB,
    .BufferCB       = _grabberCbVtbl_BufferCB
};

ISampleGrabberCB grabberCb = {
    .lpVtbl = &grabberVtbl
};

