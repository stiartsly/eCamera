#pragma once

#include <stdint.h>
#include "vsys.h"
#include "ectox.h"

struct BootstrapNode {
    char* address;
    uint16_t port;
    uint8_t key[32];
};

struct EyeTox;
struct EyeToxOps {
    int32_t (*start)    (struct EyeTox*);
    void    (*stop)     (struct EyeTox*);
    void    (*dumpId)   (struct EyeTox*);
    int32_t (*addFriend)(struct EyeTox*, const char*, const uint8_t*, int32_t);
    int32_t (*msgFriend)(struct EyeTox*, const uint8_t*, int32_t);
    int32_t (*friendIsOnline)(struct EyeTox*, int32_t*);
};

typedef void (*dispatchCb_t)(int32_t, int32_t, const uint8_t*, const uint8_t*, const uint8_t*, int32_t, int32_t, int32_t, void*);

struct EyeTox {
    int32_t friendId;
    uint8_t friendKey[32];
    int32_t friendOnline;

    char* savePath;
    int32_t running;
    struct vlock lock;

    struct ecNodeHandler* handler;
    struct ecNodeAvHandler* avHandler;
    struct EyeToxOps* ops;

    dispatchCb_t dispatchCb;
    void* dispatchCtxt;
};

struct EyeTox* eyeToxCreate(const char*, dispatchCb_t, void*);
void eyeToxDestroy(struct EyeTox*);

