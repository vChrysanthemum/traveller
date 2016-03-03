#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

typedef struct etActor_s etActor_t;
typedef struct etActor_s {
    list *channels;
    void* (*proc)(etActor_t *actor, int args, void **argv);
} etActor_t;

typedef struct etActorEvent_s {
    etActor_t *sender;
    etActor_t *receiver;
    sds     channel;

    // 信件由收件人回收
    // TODO: 考虑有多个人收件
    int     mailArgs;
    void    **mailArgv;
} etActorEvent_t;

// 推送消息给订阅者，订阅者为 etActor_t
typedef struct etChannelActor_s {
    char *key;
    list *subscribers; //etActor_t
} etChannelActor_t;

// 管理 Actor与ActorEvent
typedef struct etFactoryActor_s {
    list *actorEventPool;
    list *actorPool;
    list *runningEventList; // 正在处理中的 ActorEvent
    list *waitingEventList; // 等待处理的 ActorEvent
    dict *channels;           // 发布订阅频道 ETKeyChannelDictType
} etFactoryActor_t;

typedef struct etDeviceJob_s {
    unsigned long         index;
    pthread_t             ntid;
    const pthread_attr_t  *pthreadAttr;
    void *(*startRoutine) (void *);
    void                  *arg;
    void                  *exitStatus;
} etDeviceJob_t;

typedef void* (*etDevice_tLooper) (void* arg);
typedef struct etDevice_s etDevice_t;
typedef struct etDevice_s {
    list            *jobs;
    pthread_mutex_t jobMutex;
    pthread_mutex_t actorMutex;
    pthread_mutex_t eventMutex;
    pthread_mutex_t eventWaitMutex;
    pthread_cond_t  eventWaitCond;
    list            *waitingEventList;
    etFactoryActor_t  *factoryActor;
    pthread_t       looperNtid;
    etDevice_tLooper  looper;
    int             looperStop;
    void            *looperArg;
} etDevice_t;

typedef struct etDeviceStartJobParam_s {
    etDevice_t    *device;
    etDeviceJob_t *job;
} etDeviceStartJobParam_t;

etFactoryActor_t* ET_NewFactoryActor(void);
void ET_FreeFactoryActor(etFactoryActor_t *factoryActor);

etActorEvent_t* ET_FactoryActorNewEvent(etFactoryActor_t *factoryActor);
void ET_FactoryActorRecycleEvent(etFactoryActor_t *factoryActor,  etActorEvent_t *actorEvent);
void ET_FactoryActorAppendEvent(etFactoryActor_t *factoryActor, etActorEvent_t *actorEvent);
void ET_FactoryActorProcessEvent(etFactoryActor_t *factoryActor, etActorEvent_t *actorEvent);

etChannelActor_t* ET_NewChannelActor(void);
void ET_FreeChannelActor(etChannelActor_t *channelActor);
void ET_FactoryActorAppendChannel(etFactoryActor_t *factoryActor, etChannelActor_t *channel);
void ET_FactoryActorRemoveChannel(etFactoryActor_t *factoryActor, etChannelActor_t *channel);
void ET_SubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor);
void ET_UnSubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor);

etActor_t* ET_FactoryActorNewActor(etFactoryActor_t *factoryActor);
void ET_FactoryActorRecycleActor(etFactoryActor_t *factoryActor, etActor_t *actor); //回收Actor 

void ET_DeviceFactoryActorLoopOnce(etDevice_t *device);
void ET_DeviceFactoryActorLooper(etDevice_t *device);

void ET_FreeDeviceJob(void* _job);
etDevice_t* ET_NewDevice(etDevice_tLooper looper, void *arg);
void ET_FreeDevice(etDevice_t *device);
void ET_StartDevice(etDevice_t *device);
pthread_t ET_DeviceStartJob(etDevice_t *device, const pthread_attr_t *attr,
        void *(*startRoutine) (void *), void *arg);
void ET_DeviceAppendEvent(etDevice_t *device, etActorEvent_t *event);
list* ET_DevicePopEventList(etDevice_t *device); //弹出device上的事件
void ET_DeviceWaitEventList(etDevice_t *device); //阻塞等待事件

#endif
