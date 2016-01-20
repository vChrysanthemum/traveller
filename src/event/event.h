#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

#include <core/platform.h>
#include <pthread.h>
#include <string.h>
#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"

#define ETDEVICE_STAGE_INIT     0
#define ETDEVICE_STAGE_WAITING  1
#define ETDEVICE_STAGE_RUNNING  2

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
    list *free_actor_list;
    list *running_event_list; // 正在处理中的 ActorEvent
    list *waiting_event_list; // 等待处理的 ActorEvent
    dict *channels;           // 发布订阅频道 ETKeyChannelDictType
} ETFactoryActor;

typedef struct ETDeviceJob {
    unsigned long         index;
    pthread_t             ntid;
    const pthread_attr_t  *pthread_attr;
    void *(*start_routine) (void *);
    void                  *arg;
    void                  *exit_status;
} ETDeviceJob;

struct ETDevice;
typedef struct ETDevice ETDevice;
typedef void* (*ETDeviceLooper) (void* arg);
typedef struct ETDevice {
    int             stage;
    list            *jobs;
    pthread_mutex_t job_mutex;
    pthread_mutex_t actor_mutex;
    pthread_mutex_t event_mutex;
    pthread_mutex_t event_wait_mutex;
    pthread_cond_t  event_wait_cond;
    list            *waiting_event_list;
    ETFactoryActor  *factory_actor;
    pthread_t       looper_ntid;
    ETDeviceLooper  looper;
    int             looper_stop;
    void            *looper_arg;
} ETDevice;

typedef struct ETDeviceStartJobParam {
    ETDevice    *device;
    ETDeviceJob *job;
} ETDeviceStartJobParam;

ETActorEvent* ETNewActorEvent(void);
void ETFreeActorEvent(void *_actor);
void dictChannelDestructor(void *privdata, void *val);
ETChannelActor* ETNewChannelActor(void);
void ETFreeChannelActor(ETChannelActor *chanelActor);
ETFactoryActor* ETNewFactoryActor(void);
void ETFreeFactoryActor(ETFactoryActor *factoryActor);
ETActor* ETFactoryActorNewActor(ETFactoryActor *factoryActor);
void ETFactoryActorRecycleActor(ETFactoryActor *factoryActor, ETActor *actor); //回收Actor
void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent);
void ETFactoryActorProcessEvent(ETFactoryActor *factoryActor, ETActorEvent *event);
void ETDeviceFactoryActorLooper(ETDevice *device);

void ETFreeDeviceJob(void* _job);
ETDevice* ETNewDevice(ETDeviceLooper looper, void *arg);
void ETFreeDevice(ETDevice *device);
void ETDeviceStart(ETDevice *device);
pthread_t ETDeviceStartJob(ETDevice *device, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg);
void ETDeviceAppendEvent(ETDevice *device, ETActorEvent *event);
list* ETDevicePopEventList(ETDevice *device); //弹出device上的事件
void ETDeviceWaitEventList(ETDevice *device); //阻塞等待事件

#endif
