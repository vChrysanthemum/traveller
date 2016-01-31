#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

#include <core/platform.h>
#include <pthread.h>
#include <string.h>
#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"

struct ETActor;
typedef struct ETActor ETActor;
typedef struct ETActor {
    list *channels;
    void* (*proc)(ETActor *actor, int args, void **argv);
} ETActor;

typedef struct ETActorEvent {
    ETActor *sender;
    ETActor *receiver;
    sds     channel;

    // 信件由收件人回收
    // TODO: 考虑有多个人收件
    int     mailArgs;
    void    **mailArgv;
} ETActorEvent;

// 推送消息给订阅者，订阅者为 ETActor
typedef struct ETChannelActor {
    char *key;
    list *subscribers; //ETActor
} ETChannelActor;

// 管理 Actor与ActorEvent
typedef struct ETFactoryActor {
    list *freeActorEventList;
    list *freeActorList;
    list *runningEventList; // 正在处理中的 ActorEvent
    list *waitingEventList; // 等待处理的 ActorEvent
    dict *channels;           // 发布订阅频道 ETKeyChannelDictType
} ETFactoryActor;

typedef struct ETDeviceJob {
    unsigned long         index;
    pthread_t             ntid;
    const pthread_attr_t  *pthreadAttr;
    void *(*startRoutine) (void *);
    void                  *arg;
    void                  *exitStatus;
} ETDeviceJob;

struct ETDevice;
typedef struct ETDevice ETDevice;
typedef void* (*ETDeviceLooper) (void* arg);
typedef struct ETDevice {
    list            *jobs;
    pthread_mutex_t jobMutex;
    pthread_mutex_t actorMutex;
    pthread_mutex_t eventMutex;
    pthread_mutex_t eventWaitMutex;
    pthread_cond_t  eventWaitCond;
    list            *waitingEventList;
    ETFactoryActor  *factoryActor;
    pthread_t       looperNtid;
    ETDeviceLooper  looper;
    int             looperStop;
    void            *looperArg;
} ETDevice;

typedef struct ETDeviceStartJobParam {
    ETDevice    *device;
    ETDeviceJob *job;
} ETDeviceStartJobParam;

void dictChannelDestructor(void *privdata, void *val);

ETFactoryActor* ETNewFactoryActor(void);
void ETFreeFactoryActor(ETFactoryActor *factoryActor);

ETActorEvent* ETFactoryActorNewEvent(ETFactoryActor *factoryActor);
void ETFactoryActorRecycleEvent(ETFactoryActor *factoryActor,  ETActorEvent *actorEvent);
void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent);
void ETFactoryActorProcessEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent);

ETChannelActor* ETNewChannelActor(void);
void ETFreeChannelActor(ETChannelActor *channelActor);
void ETFactoryActorAppendChannel(ETFactoryActor *factoryActor, ETChannelActor *channel);
void ETFactoryActorRemoveChannel(ETFactoryActor *factoryActor, ETChannelActor *channel);
void ETSubscribeChannel(ETActor *actor, ETChannelActor *channelActor);
void ETUnSubscribeChannel(ETActor *actor, ETChannelActor *channelActor);

ETActor* ETFactoryActorNewActor(ETFactoryActor *factoryActor);
void ETFactoryActorRecycleActor(ETFactoryActor *factoryActor, ETActor *actor); //回收Actor 

void ETDeviceFactoryActorLoopOnce(ETDevice *device);
void ETDeviceFactoryActorLooper(ETDevice *device);

void ETFreeDeviceJob(void* _job);
ETDevice* ETNewDevice(ETDeviceLooper looper, void *arg);
void ETFreeDevice(ETDevice *device);
void ETDeviceStart(ETDevice *device);
pthread_t ETDeviceStartJob(ETDevice *device, const pthread_attr_t *attr,
        void *(*startRoutine) (void *), void *arg);
void ETDeviceAppendEvent(ETDevice *device, ETActorEvent *event);
list* ETDevicePopEventList(ETDevice *device); //弹出device上的事件
void ETDeviceWaitEventList(ETDevice *device); //阻塞等待事件

#endif
