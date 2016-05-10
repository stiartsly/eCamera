#pragma once

#include <stdint.h>
#include "ectox.h"
#include "vsys.h"

struct BootstrapNode {
    char* address;
    uint16_t port;
    uint8_t key[32];
};

struct CamTox;
struct CamToxOps {
    int32_t (*start)             (struct CamTox*);
    void    (*stop)              (struct CamTox*);
    void    (*dumpId)            (struct CamTox*);
    int32_t (*sendLossyPacket)   (struct CamTox*, const uint8_t*, int32_t);
    int32_t (*sendLosslessPacket)(struct CamTox*, const uint8_t*, int32_t);
    int32_t (*sendVideoFrame)    (struct CamTox*, const uint8_t*, const uint8_t*, const uint8_t*, int32_t, int32_t);
    int32_t (*callFriend)        (struct CamTox*);
};

struct CamTox {
    int32_t friendId;
    uint8_t friendKey[32];
    int32_t friendOnline;

    char* savePath;
    int running;
    struct vlock lock;

    ecNodeHandler* handler;
    ecNodeAvHandler* avHandler;
    struct CamToxOps* ops;

    int32_t canSendVideo;
};

struct CamTox* camToxCreate(const char* savePath);
void camToxDestroy(struct CamTox*);

