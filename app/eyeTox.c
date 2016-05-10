#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include "ectox.h"
#include "eyeTox.h"
#include "utils.h"
#include "vlog.h"
#include "vsys.h"

static
int32_t _eyeToxOpsStart(struct EyeTox* tox)
{
     if (!tox) return -1;

     vlock_enter(&tox->lock);
     if (tox->running) {
         vlock_leave(&tox->lock);
         return 0;
     }

     int32_t ret = ecNodeStart(tox->handler, tox);
     if (ret < 0) {
         vlock_leave(&tox->lock);
         vlogE("ecNodeStart failed");
         return -1;
     }

     ret = ecNodeStartAv(tox->avHandler, tox);
     if (ret < 0) {
         vlock_leave(&tox->lock);
         vlogE("ecNodeStartAv failed");
         ecNodeStop();
         return -1;
     }
     tox->running = 1;
     vlock_leave(&tox->lock);

     return ret;
}

static
void _eyeToxOpsStop(struct EyeTox* tox)
{
    if (!tox) return;

    vlock_enter(&tox->lock);
    if (tox->running) {
        ecNodeStopAv();
        ecNodeStop();
    }
    tox->running = 0;
    vlock_leave(&tox->lock);
    return ;
}

static
void _eyeToxOpsDumpId(struct EyeTox* tox)
{
    if (!tox) return ;

    uint8_t addr[ECNODE_ID_LEN];
    ecGetSelfNodeId(addr, sizeof(addr));
    toxDumpAddr(addr);
    return;
}

static
int32_t convert2NodeId(const char* hexStr, uint8_t* nodeId)
{
    assert(hexStr);
    assert(nodeId);

    if (strlen(hexStr) % 2 != 0) {
        return -1;
    }
    int   len = strlen(hexStr)/2;
    char* pos = (char*)hexStr;

    for (int i = 0; i < len; i++, pos += 2) {
        sscanf(pos, "%2hhx", &nodeId[i]);
    }
    return 0;
}

static
int32_t _eyeToxOpsAddFriend(struct EyeTox* tox, const char* peerNodeId, const uint8_t* verifyCode, int32_t length)
{
    if (!tox) return -1;
    if (!peerNodeId) return -1;
    if (!verifyCode) return -1;
    if (length <= 0) return -1;
    if (tox->friendId >= 0) return -1;

    uint8_t addr[ECNODE_ID_LEN] = {0};
    int32_t ret = convert2NodeId(peerNodeId, addr);
    if (ret < 0) {
        vlogE("Invalid hexical node ID");
        return -1;
    }
    ret = ecAddFriend(addr, verifyCode, length);
    if (ret < 0) {
        vlogE("Add friend error");
        return -1;
    }
    tox->friendId = ret;
    tox->friendOnline = 0;
    memcpy(tox->friendKey, peerNodeId, 32);
    return 0;
}

static
int32_t _eyeToxOpsMessage(struct EyeTox* tox, const uint8_t* message, int32_t length)
{
    if (!tox) return -1;
    if (!message) return -1;
    if (length <= 0) return -1;
    if (tox->friendId < 0) return -1;

    int32_t ret = ecMessageFriend(tox->friendId, message, length);
    if (ret < 0) {
        vlogE("Message to friend error");
        return -1;
    }
    return 0;
}

static
int32_t _eyeToxOpsFriendIsOnline(struct EyeTox* tox, int32_t* online)
{
    if (!tox) return -1;
    if (!online) return -1;
    if (tox->friendId < 0) return -1;

    *online = tox->friendOnline;
    return 0;
}

static
struct EyeToxOps camToxOps = {
    .start     = _eyeToxOpsStart,
    .stop      = _eyeToxOpsStop,
    .dumpId    = _eyeToxOpsDumpId,
    .addFriend = _eyeToxOpsAddFriend,
    .msgFriend = _eyeToxOpsMessage,
    .friendIsOnline = _eyeToxOpsFriendIsOnline
};

static
int32_t _handlerGetRoamingDataSize(void* ctxt)
{
    assert(ctxt);

    struct EyeTox* c = (struct EyeTox*)ctxt;
    return getSaveDataSize(c->savePath);
}

static
void _handlerLoadRoamingData(uint8_t* data, int32_t length, void* ctxt)
{
    assert(data);
    assert(length > 0);
    assert(ctxt);

    struct EyeTox* c = (struct EyeTox*)ctxt;
    (void)loadSaveData(c->savePath, data, length);
    return;
}

static
void _handlerSaveRoamingData(const uint8_t* data, int32_t length, void* ctxt)
{
    assert(data);
    assert(length > 0);
    assert(ctxt);

    struct EyeTox* c = (struct EyeTox*)ctxt;
    (void)storeSaveData(c->savePath, data, length);
    return;
}

static
void _handlerOnConnected(int32_t status, void* ctxt)
{
    assert(status > 0);
    assert(ctxt);
    (void)status;
    (void)ctxt;

    vlogI("EyeTox connected");
    return;
}

static
void _handlerOnDisconnected(void* ctxt)
{
    assert(ctxt);

    vlogI("EyeTox disconnected");
    return;
}

static
void _handlerOnBoostrap(bootstrap_t bootstrapCb, void* ctxt)
{
    assert(bootstrapCb);
    assert(ctxt);
    (void)ctxt;

    toxBootstrap(bootstrapCb);
    return;
}

static
void _handlerOnFriendConnected(int32_t friendId, void* ctxt)
{
    assert(friendId >= 0);
    assert(ctxt);

    struct EyeTox* tox = (struct EyeTox*)ctxt;

    vlock_enter(&tox->lock);
    if (tox->friendId != friendId) {
        vlock_leave(&tox->lock);
        vlogE("Not expected friend Id(%d)", friendId);
        return;
    }

    tox->friendOnline = 1;
    vlock_leave(&tox->lock);
    vlogI("Friend (%d) is connected", friendId);
    return;
}

static
void _handlerOnFriendDisconnected(int32_t friendId, void* ctxt)
{
    assert(friendId >= 0);
    assert(ctxt);

    struct EyeTox* tox = (struct EyeTox*)ctxt;

    vlock_enter(&tox->lock);
    if (tox->friendId != friendId) {
        vlock_leave(&tox->lock);
        vlogE("Not expected friend (%d) disconnected message", friendId);
        return;
    }
    tox->friendOnline = 0;
    vlock_leave(&tox->lock);
    vlogI("Friend (%d) is disconnected", friendId);
    return;
}

static
void _handlerOnFriendDeleted(int32_t friendId, void* ctxt)
{
    assert(friendId > 0);
    assert(ctxt);
    (void)friendId;
    (void)ctxt;

    vlogI("Friend (%d) deleted");
    return;
}

static
void _handlerOnLossyPacketReceived(int32_t friendId, const uint8_t* packet, int32_t length, void* ctxt)
{
    assert(friendId >= 0);
    assert(packet);
    assert(length > 0);
    assert(ctxt);
    (void)friendId;
    (void)packet;
    (void)length;
    (void)ctxt;

    vlogI("Lossy packet was received (length:%d)", length);
    return;
}

static
void _handlerOnLosslessPacketReceived(int32_t friendId, const uint8_t* packet, int32_t length, void* ctxt)
{
    assert(friendId >= 0);
    assert(packet);
    assert(length > 0);
    assert(ctxt);
    (void)friendId;
    (void)packet;
    (void)length;
    (void)ctxt;

    vlogI("Lossless packet was received (length:%d)", length);
    return;
}

static
ecNodeHandler eyeToxHandler = {
    .getRoamingDataSize       = _handlerGetRoamingDataSize,
    .loadRoamingData          = _handlerLoadRoamingData,
    .saveRoamingData          = _handlerSaveRoamingData,
    .onBootstrap              = _handlerOnBoostrap,
    .onConnected              = _handlerOnConnected,
    .onDisconnected           = _handlerOnDisconnected,
    .onFriendAddRequest       = NULL,
    .onFriendConnected        = _handlerOnFriendConnected,
    .onFriendDisconnected     = _handlerOnFriendDisconnected,
    .onFriendDeleted          = _handlerOnFriendDeleted,
    .onLossyPacketReceived    = _handlerOnLossyPacketReceived,
    .onLosslessPacketReceived = _handlerOnLosslessPacketReceived
};

static
void _handlerOnCallRequest(int32_t friendId, int32_t audio, int32_t video, void* ctxt)
{
    assert(friendId >= 0);
    assert(ctxt);

    int32_t ret = ecCallAnswer(friendId, 0, 5000);
    if (ret < 0) {
        vlogE("call answer error");
        return;
    }
    vlogI("Answer a call from friend");

    return;
}

static
void _handlerOnVideoFrameReceived(int32_t friendId, int32_t width, int32_t height,
                              const uint8_t* y, const uint8_t* u, const uint8_t* v,
                              int32_t ystride, int32_t ustride, int32_t vstride, void* ctxt)
{
    assert(friendId >= 0);
    assert(width > 0);
    assert(height > 0);
    assert(y);
    assert(u);
    assert(v);
    assert(ctxt);

    static int times = 0;

    vlogI("Received a video frame (times:%d)", ++times);

    struct EyeTox* tox = (struct EyeTox*)ctxt;
    tox->dispatchCb(width, height, y, u, v, ystride, ustride, vstride, tox->dispatchCtxt);

    return;
}

static
void _handlerOnAudioFrameReceived(int32_t friendId, const int16_t* pcm, int32_t nSamples, int32_t channels, int32_t sampleRate, void* ctxt)
{
    assert(friendId >= 0);
    assert(pcm);
    assert(nSamples > 0);
    assert(channels >= 0);
    assert(sampleRate > 0);
    assert(ctxt);

    vlogI("Received a audio frame");
    return;
}

static
ecNodeAvHandler eyeToxAvHandler = {
    .onCallRequest            = _handlerOnCallRequest,
    //.onCallStateChanged       = _handlerOnCallStateChanged,
    .onVideoFrameReceived     = _handlerOnVideoFrameReceived,
    .onAudioFrameReceived     = _handlerOnAudioFrameReceived,
};

struct EyeTox* eyeToxCreate(const char* savePath, dispatchCb_t cb, void* userData)
{
    struct EyeTox* tox = NULL;

    if (!savePath) {
        return NULL;
    }

    tox = (struct EyeTox*)calloc(1, sizeof(*tox));
    if (!tox) {
        vlogE("Calloc [EyeTox] failed");
        return NULL;
    }

    tox->savePath = strdup(savePath);
    if (!tox->savePath) {
        vlogE("Strdup [EyeTox] failed");
        free(tox);
        return NULL;
    }

    tox->dispatchCb = cb;
    tox->dispatchCtxt = userData;

    tox->ops      = &camToxOps;
    tox->handler  = &eyeToxHandler;
    tox->avHandler = &eyeToxAvHandler;

    tox->friendId = -1;
    tox->friendOnline = 0;
    tox->running  = 0;
    vlock_init(&tox->lock);

    return tox;
}

void eyeToxDestroy(struct EyeTox* tox)
{
    if (!tox) {
        return;
    }

    vlock_enter(&tox->lock);
    if (tox->running) {
        ecNodeStop();
    }
    tox->running = 0;
    vlock_leave(&tox->lock);
    vlock_deinit(&tox->lock);

    free(tox->savePath);
    free(tox);
    return;
}

