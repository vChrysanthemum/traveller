/* A simple event-driven programming library. Originally I wrote this code
 * for the Jim's event-loop (Jim is a Tcl interpreter) but later translated
 * it in form of a library for easy reuse.
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AE_H__
#define __AE_H__

#include <core/platform.h>
#include <pthread.h>
#include <string.h>
#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT 4

#define AE_NOMORE -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

#define ET_ACTOR_EVENT_TYPE_MSG 1

struct aeLooper;

/* Actor */

struct ETActor;
typedef struct ETActor ETActor;
typedef struct ETActor {
    void* (*proc)(ETActor *actor, int args, void **argv);
} ETActor;

typedef struct ETActorEvent {
    ETActor *sender;
    ETActor *receiver;
    sds     channel;
    int     mail_args;
    void    **mail_argv;
} ETActorEvent;

// 推送消息给订阅者，订阅者为 ETActor
typedef struct ETChannelActor {
    sds  key;
    list *subscribers; //ETActor
} ETChannelActor;

// 管理 Actor与ActorEvent
typedef struct ETFactoryActor {
    list *actor_list;
    list *running_event_list; // 正在处理中的 ActorEvent
    list *waiting_event_list; // 等待处理的 ActorEvent
    dict *channels;                 // 发布订阅频道 ETKeyChannelDictType
} ETFactoryActor;

ETActor* ETCreateActor(ETFactoryActor *factoryActor);
void ETFreeActor(void *_actor);
ETActorEvent* ETNewActorEvent(void);
void ETFreeActorEvent(void *_actor);
void dictChannelDestructor(void *privdata, void *val);
ETChannelActor* ETNewChannelActor(void);
void ETFreeChannelActor(ETChannelActor *chanelActor);
ETFactoryActor* ETNewFactoryActor(void);
void ETFreeFactoryActor(ETFactoryActor *factoryActor);
void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent);
void* ETFactoryActorLoopEvent(ETFactoryActor *factoryActor);

/* Types and data structures */
typedef void aeFileProc(struct aeLooper *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeLooper *eventLoop, long long id, void *clientData);
typedef void aeEventFinalizerProc(struct aeLooper *eventLoop, void *clientData);
typedef void aeBeforeSleepProc(struct aeLooper *eventLoop);

/* File event structure */
typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE) */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

/* Time event structure */
typedef struct aeTimeEvent {
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    aeTimeProc *timeProc;
    aeEventFinalizerProc *finalizerProc;
    void *clientData;
    struct aeTimeEvent *next;
} aeTimeEvent;

/* A fired event */
typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;

/* State of an event based program */
typedef struct aeLooper {
    int maxfd;   /* highest file descriptor currently registered */
    int setsize; /* max number of file descriptors tracked */
    long long timeEventNextId;
    time_t lastTime;     /* Used to detect system clock skew */
    aeFileEvent *events; /* Registered events */
    aeFiredEvent *fired; /* Fired events */
    aeTimeEvent *timeEventHead;
    int stop;
    void *apidata; /* This is used for polling API specific data */
    aeBeforeSleepProc *beforesleep;
} aeLooper;

/* Prototypes */
aeLooper *aeNewLooper(int setsize);
void aeStop(aeLooper *eventLoop);
int aeCreateFileEvent(aeLooper *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeLooper *eventLoop, int fd, int mask);
int aeGetFileEvents(aeLooper *eventLoop, int fd);
long long aeCreateTimeEvent(aeLooper *eventLoop, long long milliseconds,
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeLooper *eventLoop, long long id);
int aeProcessEvents(aeLooper *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
char *aeGetApiName(void);
void aeSetBeforeSleepProc(aeLooper *eventLoop, aeBeforeSleepProc *beforesleep);
int aeGetSetSize(aeLooper *eventLoop);
int aeResizeSetSize(aeLooper *eventLoop, int setsize);
void aeDeleteLooper(aeLooper *eventLoop);
void aeMain(aeLooper *eventLoop);
void* aeMainDeviceWrap(void *_eventLoop);

typedef struct ETDeviceJob {
    unsigned long         index;
    pthread_t             ntid;
    const pthread_attr_t  *pthread_attr;
    void *(*start_routine) (void *);
    void                  *arg;
    void                  *exit_status;
} ETDeviceJob;

typedef void* (*ETDeviceLooper) (void* arg);
typedef struct ETDevice {
    list            *jobs;
    pthread_mutex_t job_mutex;
    pthread_mutex_t actor_mutex;
    pthread_mutex_t event_mutex;
    list            *waiting_event_list;
    ETFactoryActor  *factory_actor;
    pthread_t       looper_ntid;
    ETDeviceLooper  looper;
    void            *looper_arg;
} ETDevice;

typedef struct ETDeviceStartJobParam {
    ETDevice    *device;
    ETDeviceJob *job;
} ETDeviceStartJobParam;

void ETFreeDeviceJob(void* _job);
ETDevice* ETNewDevice(ETDeviceLooper looper, void *arg);
void ETFreeDevice(ETDevice *device);
void ETDeviceStart(ETDevice *device);
pthread_t ETDeviceStartJob(ETDevice *device, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg);
void ETDeviceAppendEvent(ETDevice *device, ETActorEvent *event);
list* ETDevicePopEventList(ETDevice *device);

#endif
