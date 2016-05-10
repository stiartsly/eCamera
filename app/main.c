#include <stdio.h>
#include <stdlib.h>
#include "eyeTox.h"
#include "eyeWin.h"

static
void yuv420tobgr(uint16_t width, uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v, unsigned int ystride, unsigned int ustride, unsigned int vstride, uint8_t *out)
{
    unsigned long int i, j;
    for (i = 0; i < height; ++i) {
        for (j = 0; j < width; ++j) {
            uint8_t *point = out + 4 * ((i * width) + j);
            int t_y = y[((i * ystride) + j)];
            int t_u = u[(((i / 2) * ustride) + (j / 2))];
            int t_v = v[(((i / 2) * vstride) + (j / 2))];
            t_y = t_y < 16 ? 16 : t_y;

            int r = (298 * (t_y - 16) + 409 * (t_v - 128) + 128) >> 8;
            int g = (298 * (t_y - 16) - 100 * (t_u - 128) - 208 * (t_v - 128) + 128) >> 8;
            int b = (298 * (t_y - 16) + 516 * (t_u - 128) + 128) >> 8;

            point[2] = r>255? 255 : r<0 ? 0 : r;
            point[1] = g>255? 255 : g<0 ? 0 : g;
            point[0] = b>255? 255 : b<0 ? 0 : b;
            point[3] = ~0;
        }
    }
}

static
void renderVideoFrame(int32_t width, int32_t height,
                        const uint8_t* y, const uint8_t* u, const uint8_t* v,
                        int32_t ystride, int32_t ustride, int32_t vstride,
                        void* userData)
{
    uint8_t* frame = calloc(1, width * height * 4);
    if (!frame) {
        printf("calloc problem.\n");
        return;
    }

    yuv420tobgr(width, height, y, u, v, ystride, ustride, vstride, frame);

    struct EyeWin* win = (struct EyeWin*)userData;
    win->ops->renderFrame(win, width, height, frame);

    free(frame);
    return;
}

int main(int argc, char** argv)
{
    struct EyeTox* tox = NULL;
    int ret = 0;

    if (argc != 2) {
        printf("Help: %s PUBLIC_KEY\n", argv[0]);
        exit(-1);
    }
    struct EyeWin* win = eyeWinCreate();
    if (!win) {
        printf("Create EyeWin failed.\n");
        goto errorExit;
    }

    tox = eyeToxCreate("./eye_data", renderVideoFrame, win);
    if (!tox) {
        printf("Create eyeTox failed\n");
        goto errorExit;
    }

    win->ops->start(win);
    tox->ops->start(tox);
    tox->ops->dumpId(tox);

    tox->ops->addFriend(tox, argv[1], (const uint8_t*)"123456", 6);
    int32_t online = 0;
    tox->ops->friendIsOnline(tox, &online);
    while(!online) {
        Sleep(500);
        tox->ops->friendIsOnline(tox, &online);
    }
    tox->ops->msgFriend(tox, (const uint8_t*)"start", sizeof("start"));

    int32_t times = 0;
    while(1) {
        Sleep(1000);
        if (times < 2) {
            tox->ops->msgFriend(tox, (const uint8_t*)&times, sizeof(times));
        }
        ++times;
    }

    tox->ops->stop(tox);
    win->ops->stop(win);
    eyeToxDestroy(tox);
    eyeWinDestroy(win);

    return 0;

errorExit:
    if (win) eyeWinDestroy(win);
    if (tox) eyeToxDestroy(tox);
    return -1;
}

