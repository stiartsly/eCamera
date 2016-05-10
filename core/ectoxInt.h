#pragma once

#include <stdint.h>
#include "tox/tox.h"
#include "tox/toxav.h"
#include "ectox.h"
#include "vsys.h"

enum {
    msgFriendRequest = (int32_t)0x01,
    msgFriendMessage = (int32_t)0x02,
    msgFriendConnectionMessage,

    msgFriendLossyPacket,
    msgFriendLosslessPacket
};

enum {
    msgFriendRequestCall = (int32_t)0x01,
    msgFriendCallStateChanged,
    msgFriendAudioFrame,
    msgFriendVideoFrame,

};

struct ToxMsg {
    uint8_t key[TOX_ADDRESS_SIZE];
    uint32_t friendId;
    int32_t msgId;
    void* data;
    int32_t length;
    int32_t status;
    void* cookie;
};
typedef struct ToxMsg ToxMsg;

struct ToxAvMsg {
    uint32_t friendId;
    int32_t msgId;
    int32_t audio;
    int32_t video;
    void* data;
    int32_t length;
    uint32_t status;
    void* cookie;
};
typedef struct ToxAvMsg ToxAvMsg;

struct ToxNode {
    Tox* tox;

    ecNodeHandler* handler;
    void* userData;

    struct vthread thread;
    int32_t threadQuit;

    ToxMsg msg;
};

struct ToxAvNode {
    ToxAV* av;

    ecNodeAvHandler* handler;
    void* userData;

    struct vthread thread;
    int32_t threadQuit;

    ToxAvMsg msg;
};


typedef struct ToxNode   ToxNode;
typedef struct ToxAvNode ToxAvNode;

