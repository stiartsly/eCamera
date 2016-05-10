#include <stdio.h>
#include <Windows.h>
#include "obsolete.h"
#include <vpx/vpx_codec.h>
#include <vpx/vpx_image.h>
#include "camera.h"
#include "camTox.h"

static
uint8_t rgb_to_y(int r, int g, int b)
{
    int y = ((9798 * r + 19235 * g + 3736 * b) >> 15);
    return y>255? 255 : y<0 ? 0 : y;
}

static
uint8_t rgb_to_u(int r, int g, int b)
{
    int u = ((-5538 * r + -10846 * g + 16351 * b) >> 15) + 128;
    return u>255? 255 : u<0 ? 0 : u;
}

static
uint8_t rgb_to_v(int r, int g, int b)
{
    int v = ((16351 * r + -13697 * g + -2664 * b) >> 15) + 128;
    return v>255? 255 : v<0 ? 0 : v;
}

static
void bgrtoyuv420(uint8_t *plane_y, uint8_t *plane_u, uint8_t *plane_v, uint8_t *rgb, uint16_t width, uint16_t height)
{
    uint16_t x, y;
    uint8_t *p;
    uint8_t r, g, b;

    for(y = 0; y != height; y += 2) {
        p = rgb;
        for(x = 0; x != width; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);
        }

        for(x = 0; x != width / 2; x++) {
            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b = *rgb++;
            g = *rgb++;
            r = *rgb++;
            *plane_y++ = rgb_to_y(r, g, b);

            b = ((int)b + (int)*(rgb - 6) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            g = ((int)g + (int)*(rgb - 5) + (int)*p + (int)*(p + 3) + 2) / 4; p++;
            r = ((int)r + (int)*(rgb - 4) + (int)*p + (int)*(p + 3) + 2) / 4; p++;

            *plane_u++ = rgb_to_u(r, g, b);
            *plane_v++ = rgb_to_v(r, g, b);

            p += 3;
        }
    }
}

static
void dispatchFrameCb(int32_t width, int32_t height, const uint8_t* buffer, int32_t length, void* ctxt)
{
    struct CamTox* tox = (struct CamTox*)ctxt;
    static uint8_t* frame = NULL;
    if (!frame) {
        frame = calloc(1, width * height * 3);
        if (!frame) return;
    }

    static vpx_image_t input;
    static int alloced = 0;
    static uint8_t* y = NULL;
    static uint8_t* u = NULL;
    static uint8_t* v = NULL;
    static uint16_t w = 0;
    static uint16_t h = 0;

    if (!alloced) {
        vpx_img_alloc(&input, VPX_IMG_FMT_I420, width, height, 1);
        y = input.planes[0];
        u = input.planes[1];
        v = input.planes[2];
        w = input.d_w;
        h = input.d_h;
        alloced = 1;
    }

    if ((w != width) || (h != height)) {
        printf(" width, height mismatch.\n");
        return;
    }

    uint8_t* dst = frame + width * height * 3;
    uint8_t* src = (uint8_t*) buffer;
    int32_t  sz  = width * 3;
    for (int i = 0; i < height; i++) {
        dst -= sz;
        memcpy(dst, src, sz);
        src += sz;
    }
    bgrtoyuv420(y, u, v, frame, width, height);
    tox->ops->sendVideoFrame(tox, y, u, v, w, h);

    return;
}

int main(int argc, char** argv)
{
    struct Camera* cam = NULL;
    struct CamTox* tox = NULL;
    int ret = 0;

    tox = camToxCreate("./camera_data");
    if (!tox) {
        printf("Create camTox failed\n");
        goto errorExit;
    }
    cam = cameraCreate(dispatchFrameCb, tox);
    if (!cam) {
        printf("Create camera failed\n");
        goto errorExit;
    }
    ret = cam->ops->startCapture(cam);
    if (ret < 0) {
        printf("Start capture failed\n");
        goto errorExit;
    }
    tox->ops->start(tox);
    tox->ops->dumpId(tox);
    while (1) {
        Sleep(100);
    }
    tox->ops->stop(tox);
    cam->ops->stopCapture(cam);
    camToxDestroy(tox);
    cameraDestroy(cam);

    return 0;

errorExit:
    if (tox) camToxDestroy(tox);
    if (cam) cameraDestroy(cam);
    return -1;
}

