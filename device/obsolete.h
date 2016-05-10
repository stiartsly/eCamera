#pragma once
#include <Windows.h>
#include <Strmif.h>

typedef interface ISampleGrabberCB ISampleGrabberCB;
typedef interface ISampleGrabber ISampleGrabber;

typedef struct ISampleGrabberCBVtbl {
    BEGIN_INTERFACE
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(ISampleGrabberCB * This, REFIID riid, _COM_Outptr_  void **ppvObject);
    ULONG  (STDMETHODCALLTYPE *AddRef)        (ISampleGrabberCB * This);
    ULONG  (STDMETHODCALLTYPE *Release)       (ISampleGrabberCB * This);
    HRESULT(STDMETHODCALLTYPE *SampleCB)      (ISampleGrabberCB * This, double SampleTime, IMediaSample *pSample);
    HRESULT(STDMETHODCALLTYPE *BufferCB)      (ISampleGrabberCB * This, double SampleTime, BYTE *pBuffer, long BufferLen);
    END_INTERFACE
} ISampleGrabberCBVtbl;

interface ISampleGrabberCB {
    CONST_VTBL struct ISampleGrabberCBVtbl *lpVtbl;
};

typedef struct ISampleGrabberVtbl {
    BEGIN_INTERFACE
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(ISampleGrabber * This, REFIID riid,  _COM_Outptr_  void **ppvObject);
    ULONG(STDMETHODCALLTYPE *AddRef)          (ISampleGrabber * This);
    ULONG(STDMETHODCALLTYPE *Release)         (ISampleGrabber * This);
    HRESULT(STDMETHODCALLTYPE *SetOneShot)    (ISampleGrabber * This, BOOL OneShot);
    HRESULT(STDMETHODCALLTYPE *SetMediaType)  (ISampleGrabber * This, const AM_MEDIA_TYPE *pType);
    HRESULT(STDMETHODCALLTYPE *GetConnectedMediaType)(ISampleGrabber * This, AM_MEDIA_TYPE *pType);
    HRESULT(STDMETHODCALLTYPE *SetBufferSamples)     (ISampleGrabber * This, BOOL BufferThem);
    HRESULT(STDMETHODCALLTYPE *GetCurrentBuffer)     (ISampleGrabber * This, long *pBufferSize, long *pBuffer);
    HRESULT(STDMETHODCALLTYPE *GetCurrentSample)     (ISampleGrabber * This, IMediaSample **ppSample);
    HRESULT(STDMETHODCALLTYPE *SetCallback)   (ISampleGrabber * This, ISampleGrabberCB *pCallback, long WhichMethodToCallback);
    END_INTERFACE
} ISampleGrabberVtbl;

interface ISampleGrabber{
    CONST_VTBL struct ISampleGrabberVtbl *lpVtbl;
};

#if 1
EXTERN_C const CLSID CLSID_SampleGrabber;
EXTERN_C const CLSID CLSID_NullRenderer;
EXTERN_C const IID IID_ISampleGrabber;
EXTERN_C const IID CLSID_NullRenderer;
#endif

DEFINE_GUID(CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3, 0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37);

