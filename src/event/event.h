#ifndef __EVENT_EVENT_H__
#define __EVENT_EVENT_H__

typedef struct etActor_s etActor_t;
typedef struct etActor_s {
    list *Channels;
    void* (*Proc)(etActor_t *actor, int args, void **argv);
} etActor_t;

typedef struct etActorEvent_s {
    etActor_t *Sender;
    etActor_t *Receiver;
    sds       Channel;

    // 信件由收件人回收
    // TODO: 考虑有多个人收件
    int     MailArgs;
    void    **MailArgv;
} etActorEvent_t;

// 推送消息给订阅者，订阅者为 etActor_t
typedef struct etChannelActor_s {
    char *Key;
    list *Subscribers; //etActor_t
} etChannelActor_t;

// 管理 Actor与ActorEvent
typedef struct etFactoryActor_s {
    list *ActorEventPool;
    list *ActorPool;
    list *RunningEventList; // 正在处理中的 ActorEvent
    list *WaitingEventList; // 等待处理的 ActorEvent
    dict *Channels;           // 发布订阅频道 ETKeyChannelDictType
} etFactoryActor_t;

typedef struct etDeviceJob_s {
    pthread_t             ntid;
    const pthread_attr_t  *pthreadAttr;
    unsigned long         Index;
    void *(*StartRoutine) (void *);
    void                  *Arg;
    void                  *ExitStatus;
} etDeviceJob_t;

typedef void* (*etDevice_tLooper) (void* arg);
typedef struct etDevice_s etDevice_t;
typedef struct etDevice_s {
    pthread_mutex_t   jobMutex;
    pthread_mutex_t   actorMutex;
    pthread_mutex_t   eventMutex;
    pthread_mutex_t   eventWaitMutex;
    pthread_cond_t    eventWaitCond;
    list              *Jobs;
    list              *WaitingEventList;
    etFactoryActor_t  *FactoryActor;
    pthread_t         LooperNtid;
    etDevice_tLooper  Looper;
    int               LooperStop;
    void              *LooperArg;
} etDevice_t;

typedef struct etDeviceStartJobParam_s {
    etDevice_t    *Device;
    etDeviceJob_t *Job;
} etDeviceStartJobParam_t;

etFactoryActor_t* ET_NewFactoryActor(void);
void ET_FreeFactoryActor(etFactoryActor_t *FactoryActor);

etActorEvent_t* ET_FactoryActorNewEvent(etFactoryActor_t *FactoryActor);
void ET_FactoryActorRecycleEvent(etFactoryActor_t *FactoryActor,  etActorEvent_t *actorEvent);
void ET_FactoryActorAppendEvent(etFactoryActor_t *FactoryActor, etActorEvent_t *actorEvent);
void ET_FactoryActorProcessEvent(etFactoryActor_t *FactoryActor, etActorEvent_t *actorEvent);

etChannelActor_t* ET_NewChannelActor(void);
void ET_FreeChannelActor(etChannelActor_t *channelActor);
void ET_FactoryActorAppendChannel(etFactoryActor_t *FactoryActor, etChannelActor_t *channel);
void ET_FactoryActorRemoveChannel(etFactoryActor_t *FactoryActor, etChannelActor_t *channel);
void ET_SubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor);
void ET_UnSubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor);

etActor_t* ET_FactoryActorNewActor(etFactoryActor_t *FactoryActor);
void ET_FactoryActorRecycleActor(etFactoryActor_t *FactoryActor, etActor_t *actor); //回收Actor 

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
