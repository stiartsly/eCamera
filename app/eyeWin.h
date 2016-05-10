#pragma once

#include <stdint.h>
#include <Windows.h>
#include "vsys.h"

struct EyeWin;
struct EyeWinOps {
    int32_t (*start)      (struct EyeWin*);
    void    (*stop)       (struct EyeWin*);
    void    (*renderFrame)(struct EyeWin*, int32_t, int32_t, const uint8_t*);

};

struct EyeWin {
    int32_t width;
    int32_t height;

    struct vcond cond;
    struct vlock lock;
    struct vthread thread;

    HWND hwnd;
    HINSTANCE hInst;
    struct EyeWinOps* ops;
};

struct EyeWin* eyeWinCreate(void);
void eyeWinDestroy(struct EyeWin*);

