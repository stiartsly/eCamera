#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ectox.h"
#include "camTox.h"
#include "vlog.h"
#include "vsys.h"
#include "utils.h"

static
int32_t _camToxOpsStart(struct CamTox* tox)
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
         ecNodeStop();
         vlogE("ecNodeAvStart failed");
         return -1;
     }
     tox->running = 1;
     vlock_leave(&tox->lock);

     return ret;
}

static
void _camToxOpsStop(struct CamTox* tox)
{
    if (!tox) return;

    vlock_enter(&tox->lock);
    if (tox->running) {
        ecNodeStopAv();
        ecNodeStop();
    }
    tox->running = 1;
    vlock_leave(&tox->lock);
    return ;
}

static
void _camToxOpsDumpId(struct CamTox* tox)
{
    if (!tox) return ;

    uint8_t addr[ECNODE_ID_LEN];
    ecGetSelfNodeId(addr, sizeof(addr));
    toxDumpAddr(addr);
    return;
}

static
int32_t _camToxOpsSendLossyPacket(struct CamTox* tox, const uint8_t* data, int length)
{
    if (!tox) return -1;
    if (!data) return -1;
    if (length <= 0) return -1;
    if (tox->friendId < 0) return -1;

    uint8_t* packet = calloc(1, length + 1);
    if (!packet) {
        vlogE("calloc failed");
        return -1;
    }
    packet[0] = (uint8_t)201;
    memcpy(packet + 1, data, length);
    int32_t ret = ecSendLossyPacket(tox->friendId, packet, length + 1);
    if (ret < 0) {
        vlogE("send lossy packet error");
        return -1;
    }
    return ret;
}

static
int32_t _camToxOpsSendLosslessPacket(struct CamTox* tox, const uint8_t* data, int length)
{
    if (!tox) return -1;
    if (!data) return -1;
    if (length <= 0) return -1;
    if (tox->friendId < 0) return -1;

    int32_t ret = ecSendLosslessPacket(tox->friendId, data, length);
    if (ret < 0) {
        vlogE("send lossless packet error");
        return -1;
    }
    return ret;
}

static
int32_t _camToxOpsCallFriend(struct CamTox* tox)
{
    if (!tox) return -1;
    if (tox->friendId < 0) return -1;
    if (!tox->friendOnline) return -1;

    int32_t ret = ecCallFriend(tox->friendId, 0, 5000);
    if (ret < 0) {
        vlogE("call friend error");
        return -1;
    }
    return 0;
}

static
int32_t _camToxOpsSendVideoFrame(struct CamTox* tox, const uint8_t* y, const uint8_t* u, const uint8_t* v, int32_t width, int32_t height)
{
    if (!tox) return -1;
    if (!y) return -1;
    if (!u) return -1;
    if (!v) return -1;
    if (width <= 0) return -1;
    if (height <= 0) return -1;
    if (tox->friendId < 0) return -1;
    if (!tox->friendOnline) return -1;
    if (!tox->canSendVideo) return -1;

    int32_t ret = ecSendVideoFrame(tox->friendId, y, u, v, width, height);
    if (ret < 0) {
        vlogE("Send video frame error");
        return -1;
    }
    vlogI("Send a video frame to remote eyer");
    return 0;
}

static
struct CamToxOps camToxOps = {
    .start              = _camToxOpsStart,
    .stop               = _camToxOpsStop,
    .dumpId             = _camToxOpsDumpId,
    .sendLossyPacket    = _camToxOpsSendLossyPacket,
    .sendLosslessPacket = _camToxOpsSendLosslessPacket,
    .sendVideoFrame     = _camToxOpsSendVideoFrame,
    .callFriend         = _camToxOpsCallFriend,
};

static
int32_t _handlerGetRoamingDataSize(void* ctxt)
{
    assert(ctxt);

    struct CamTox* c = (struct CamTox*)ctxt;
    return getSaveDataSize(c->savePath);
}

static
void _handlerLoadRoamingData(uint8_t* data, int32_t length, void* ctxt)
{
    assert(data);
    assert(length > 0);
    assert(ctxt);

    struct CamTox* c = (struct CamTox*)ctxt;
    (void)loadSaveData(c->savePath, data, length);
    return;
}

static
void _handlerSaveRoamingData(const uint8_t* data, int32_t length, void* ctxt)
{
    assert(data);
    assert(length > 0);
    assert(ctxt);

    struct CamTox* c = (struct CamTox*)ctxt;
    (void)storeSaveData((const char*)c->savePath, data, length);
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
void _handlerOnConnected(int32_t status, void* ctxt)
{
    assert(status > 0);
    assert(ctxt);
    (void)status;
    (void)ctxt;

    vlogI("CamTox connected");
    return;
}

static
void _handlerOnDisconnected(void* ctxt)
{
    assert(ctxt);

    vlogI("CamTox disconnected");
    return;
}

static
void _handlerOnAddRequest(const uint8_t* peerNodeId, const uint8_t* verifyCode, int32_t length, void* ctxt)
{
    assert(peerNodeId);
    assert(verifyCode);
    assert(length > 0);
    assert(ctxt);

    vlogI("Friend Add request");

    struct CamTox* c = (struct CamTox*)ctxt;
    if (c->friendId >= 0) {
        vlogE("Reject friend::Already has a friend");
        return;
    }

    char* credCode = "123456";
    if ((strlen(credCode) != length) || memcmp(verifyCode, credCode, length)) {
        vlogE("Reject friend::verify code mismatched");
        return;
    }

    int32_t friendId = ecAcceptFriend(peerNodeId);
    if (friendId < 0) {
        vlogE("Reject Friend::ecAcceptFriend error");
        return;
    }

    vlock_enter(&c->lock);
    c->friendId  = friendId;
    c->friendOnline = 0;
    memcpy(c->friendKey, peerNodeId, 32);
    vlock_leave(&c->lock);

    vlogI("Accepted Friend (friendId:%d)", friendId);
    return;
}

static
void _handlerOnFriendMessage(int32_t friendId, const uint8_t* message, int32_t length, void* ctxt)
{
    assert(friendId >= 0);
    assert(message);
    assert(length >= 0);
    assert(ctxt);

    vlogI("Friend Message");

    struct CamTox* c = (struct CamTox*)ctxt;
    if (c->friendId != friendId) {
        vlogE("Invalid friend message (friendId:%d)\n", friendId);
        return;
    }
    if (length != sizeof(int32_t)) {
        vlogE("Invalid message (length:%d)\n", length);
        return;
    }
    int32_t* times = (int32_t*)message;
    vlogI("times:%d", *times);

    return;
}

static
void _handlerOnFriendConnected(int32_t friendId, void* ctxt)
{
    assert(friendId >= 0);
    assert(ctxt);

    struct CamTox* c = (struct CamTox*)ctxt;

    vlock_enter(&c->lock);
    if (c->friendId != friendId) {
       vlock_leave(&c->lock);
       vlogE("Invalid friend's connected message");
       return;
    }

    c->friendOnline = 1;
    vlock_leave(&c->lock);
    vlogI("Friend (%d) is connected", friendId);

    int32_t ret = c->ops->callFriend(c);
    if (ret < 0) {
        vlogE(" call friend error");
        return;
    }
    vlogI("Call Friending --->");
    return;
}

static
void _handlerOnFriendDisconnected(int32_t friendId, void* ctxt)
{
    assert(friendId >= 0);
    assert(ctxt);

    struct CamTox* c = (struct CamTox*)ctxt;

    vlock_enter(&c->lock);
    if (c->friendId != friendId) {
       vlock_leave(&c->lock);
       vlogE("Invalid friend's disconnected message");
       return;
    }
    c->friendOnline = 0;
    vlock_leave(&c->lock);
    vlogI("Friend (%d) is disconnected", friendId);
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
ecNodeHandler camNodeHandler = {
    .getRoamingDataSize       = _handlerGetRoamingDataSize,
    .loadRoamingData          = _handlerLoadRoamingData,
    .saveRoamingData          = _handlerSaveRoamingData,
    .onBootstrap              = _handlerOnBoostrap,
    .onConnected              = _handlerOnConnected,
    .onDisconnected           = _handlerOnDisconnected,
    .onFriendAddRequest       = _handlerOnAddRequest,
    .onFriendConnected        = _handlerOnFriendConnected,
    .onFriendDisconnected     = _handlerOnFriendDisconnected,
    .onFriendDeleted          = NULL,
    .onFriendMessage          = _handlerOnFriendMessage,
    .onLossyPacketReceived    = _handlerOnLossyPacketReceived,
    .onLosslessPacketReceived = _handlerOnLosslessPacketReceived
};

static
void _handlerOnCallStateChanged(int32_t friendId, uint32_t state, void* ctxt)
{
     assert(friendId >= 0);
     assert(ctxt);

     vlogI("On State Changed: state :0x%x\n", state);
     struct CamTox* tox = (struct CamTox*)ctxt;

     tox->canSendVideo = 1;
     return;
}

static
ecNodeAvHandler camNodeAvHandler = {
    .onCallRequest            = NULL,
    .onCallStateChanged       = _handlerOnCallStateChanged,
    .onAudioFrameReceived     = NULL,
    .onVideoFrameReceived     = NULL
};

struct CamTox* camToxCreate(const char* savePath)
{
    struct CamTox* c = NULL;

    if (!savePath) {
         return NULL;
    }

    c = (struct CamTox*)calloc(1, sizeof(*c));
    if (!c) {
        vlogE("Calloc [CamTox] failed");
        return NULL;
    }
    c->savePath = strdup(savePath);
    if (!c->savePath) {
        vlogE("Strdup [CamTox] failed");
        free(c);
        return NULL;
    }

    c->ops        = &camToxOps;
    c->handler    = &camNodeHandler;
    c->avHandler  = &camNodeAvHandler;

    c->friendId   = -1;
    c->friendOnline = 0;
    c->canSendVideo = 0;
    vlock_init(&c->lock);

    return c;
}

void camToxDestroy(struct CamTox* tox)
{
    if (!tox) {
        return;
    }

    vlock_enter(&tox->lock);
    if (tox->running) {
        ecNodeStop();
    }
    vlock_leave(&tox->lock);
    vlock_deinit(&tox->lock);

    free(tox->savePath);
    free(tox);
    return;
}

