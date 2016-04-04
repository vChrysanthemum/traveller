#include <pthread.h>

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/zmalloc.h"
#include "event/event.h"

static void freeActorEvent(etActorEvent_t *event);

static void dictChannelDestructor(void *privdata, void *val) {
    DICT_NOTUSED(privdata);
    ET_FreeChannelActor((etChannelActor_t*)val);
}

dictType ETKeyChannelDictType = {
    dictStringHash,             /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictStringCompare,          /* key compare */
    dictStringDestructor,       /* key destructor */
    dictChannelDestructor       /* val destructor */
};

etFactoryActor_t* ET_NewFactoryActor(void) {
    etFactoryActor_t *FactoryActor = (etFactoryActor_t*)zmalloc(sizeof(etFactoryActor_t));
    memset(FactoryActor, 0, sizeof(etFactoryActor_t));

    FactoryActor->ActorEventPool = listCreate();

    FactoryActor->ActorPool = listCreate();

    FactoryActor->RunningEventList = listCreate();

    FactoryActor->WaitingEventList = listCreate();

    FactoryActor->Channels = dictCreate(&ETKeyChannelDictType, NULL);

    return FactoryActor;
}

void ET_FreeFactoryActor(etFactoryActor_t *FactoryActor) {
    listIter *li;
    listNode *ln;
    li = listGetIterator(FactoryActor->ActorEventPool, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        freeActorEvent((etActorEvent_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);

    listRelease(FactoryActor->ActorPool);
    li = listGetIterator(FactoryActor->ActorPool, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        zfree(ln->Value);
    }
    listReleaseIterator(li);
    listRelease(FactoryActor->ActorPool);

    li = listGetIterator(FactoryActor->RunningEventList, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        freeActorEvent((etActorEvent_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);
    listRelease(FactoryActor->RunningEventList);

    li = listGetIterator(FactoryActor->WaitingEventList, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        freeActorEvent((etActorEvent_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);
    listRelease(FactoryActor->WaitingEventList);

    zfree(FactoryActor);
}

static inline void freeActorEvent(etActorEvent_t *actorEvent) {
    sdsfree(actorEvent->Channel);
    zfree(actorEvent);
}

static inline void resetActorEvent(etActorEvent_t *actorEvent) {
    sdsfree(actorEvent->Channel);
}

etActorEvent_t* ET_FactoryActorNewEvent(etFactoryActor_t *FactoryActor) {
    etActorEvent_t *actorEvent;

    if (0 == listLength(FactoryActor->ActorEventPool)) {
        for (int i = 0; i < 20; i++) {
            actorEvent = (etActorEvent_t*)zmalloc(sizeof(etActorEvent_t));
            memset(actorEvent, 0, sizeof(etActorEvent_t));

            FactoryActor->ActorEventPool = listAddNodeTail(FactoryActor->ActorEventPool, actorEvent);
        }
    }

    listNode *ln = listFirst(FactoryActor->ActorEventPool);
    actorEvent = (etActorEvent_t*)ln->Value;
    listDelNode(FactoryActor->ActorEventPool, ln);

    memset(actorEvent, 0, sizeof(etActorEvent_t));

    return actorEvent;
}

void ET_FactoryActorRecycleEvent(etFactoryActor_t *FactoryActor,  etActorEvent_t *actorEvent) {
    if (listLength(FactoryActor->ActorEventPool) > 200) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(FactoryActor->ActorEventPool, AL_START_HEAD);
        for (int i = 0; i < 100 && 0 != (ln = listNext(li)); i++) {
            freeActorEvent((etActorEvent_t*)listNodeValue(ln));
            listDelNode(FactoryActor->ActorEventPool, ln);
        }
        listReleaseIterator(li);
    }

    resetActorEvent(actorEvent);
    FactoryActor->ActorEventPool = listAddNodeTail(FactoryActor->ActorEventPool, actorEvent);
}

void ET_FactoryActorAppendEvent(etFactoryActor_t *FactoryActor, etActorEvent_t *actorEvent) {
    FactoryActor->WaitingEventList = listAddNodeTail(FactoryActor->WaitingEventList, actorEvent);
}

void ET_FactoryActorProcessEvent(etFactoryActor_t *FactoryActor, etActorEvent_t *event) {
    etActor_t *actor;
    etChannelActor_t *channel;
    listIter *liActor;
    listNode *lnActor;

    if (0 != event->Receiver) {
        actor = event->Receiver;
        actor->Proc(actor, event->MailArgs, event->MailArgv);
    }

    if (0 != sdslen(event->Channel)) {
        channel = (etChannelActor_t*)dictFetchValue(FactoryActor->Channels, event->Channel);

        if (0 != channel) {
            liActor = listGetIterator(channel->Subscribers, AL_START_HEAD);
            while (0 != (lnActor = listNext(liActor))) {
                actor = (etActor_t*)lnActor->Value;
                actor->Proc(actor, event->MailArgs, event->MailArgv);
            }
            listReleaseIterator(liActor);
        }
    }
}

etChannelActor_t *ET_NewChannelActor(void) {
    etChannelActor_t *channelActor = (etChannelActor_t*)zmalloc(sizeof(etChannelActor_t));
    memset(channelActor, 0, sizeof(etChannelActor_t));
    channelActor->Subscribers = listCreate();
    return channelActor;
}

void ET_FreeChannelActor(etChannelActor_t *channelActor) {
    zfree(channelActor->Key);
    listRelease(channelActor->Subscribers);
}

void ET_FactoryActorAppendChannel(etFactoryActor_t *FactoryActor, etChannelActor_t *channel) {
    dictAdd(FactoryActor->Channels, stringnew(channel->Key), channel);
}

void ET_FactoryActorRemoveChannel(etFactoryActor_t *FactoryActor, etChannelActor_t *channel) {
    dictDelete(FactoryActor->Channels, channel->Key);
}

void ET_SubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor) {
    if (0 == actor->Channels) {
        actor->Channels = listCreate();
    }
    actor->Channels = listAddNodeTail(actor->Channels, channelActor);
    channelActor->Subscribers = listAddNodeTail(channelActor->Subscribers, actor);
}

void ET_UnSubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor) {
    listIter *li;
    listNode *ln;

    li = listGetIterator(actor->Channels, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        if ((void*)channelActor == listNodeValue(ln)) {
            listDelNode(actor->Channels, ln);
            break;
        }
    }
    listReleaseIterator(li);

    li = listGetIterator(channelActor->Subscribers, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        if ((void*)actor == listNodeValue(ln)) {
            listDelNode(channelActor->Subscribers, ln);
            break;
        }
    }
    listReleaseIterator(li);
}

static inline void freeActorChannels(etActor_t *actor) {
    if (0 != actor->Channels) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(actor->Channels, AL_START_HEAD);
        while (0 != (ln = listNext(li))) {
            ET_UnSubscribeChannel(actor, (etChannelActor_t*)listNodeValue(ln));
        }
        listReleaseIterator(li);
        listRelease(actor->Channels);
    }
}

static inline void freeActor(etActor_t *actor) {
    freeActorChannels(actor);
    zfree(actor);
}

static inline void resetActor(etActor_t *actor) {
    freeActorChannels(actor);
}

etActor_t* ET_FactoryActorNewActor(etFactoryActor_t *FactoryActor) {
    etActor_t *actor;

    if (0 == listLength(FactoryActor->ActorPool)) {
        for (int i = 0; i < 20; i++) {
            actor = (etActor_t*)zmalloc(sizeof(etActor_t));
            memset(actor, 0, sizeof(etActor_t));

            FactoryActor->ActorPool = listAddNodeTail(FactoryActor->ActorPool, actor);
        }
    }

    listNode *ln = listFirst(FactoryActor->ActorPool);
    actor = (etActor_t*)ln->Value;
    listDelNode(FactoryActor->ActorPool, ln);

    memset(actor, 0, sizeof(etActor_t));

    return actor;
}

void ET_FactoryActorRecycleActor(etFactoryActor_t *FactoryActor, etActor_t *actor) {
    if (listLength(FactoryActor->ActorPool) > 200) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(FactoryActor->ActorPool, AL_START_HEAD);
        for (int i = 0; i < 100 && 0 != (ln = listNext(li)); i++) {
            freeActor((etActor_t*)listNodeValue(ln));
            listDelNode(FactoryActor->ActorPool, ln);
        }
        listReleaseIterator(li);
    }

    resetActor(actor);
    FactoryActor->ActorPool = listAddNodeTail(FactoryActor->ActorPool, actor);
}

void ET_DeviceFactoryActorLoopOnce(etDevice_t *device) {
    etFactoryActor_t *FactoryActor = device->FactoryActor;
    list *_l;
    listIter *li;
    listNode *ln;

    if (0 == listLength(FactoryActor->RunningEventList)) {
        _l = FactoryActor->RunningEventList;
        FactoryActor->RunningEventList = FactoryActor->WaitingEventList;
        FactoryActor->WaitingEventList = _l;
    }

    li = listGetIterator(FactoryActor->RunningEventList, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        ET_FactoryActorProcessEvent(FactoryActor, (etActorEvent_t*)ln->Value);

        ET_FactoryActorRecycleEvent(FactoryActor, (etActorEvent_t*)ln->Value);
        listDelNode(FactoryActor->RunningEventList, ln);
    }
    listReleaseIterator(li);

    _l = ET_DevicePopEventList(device);
    if (0 != _l) {
        li = listGetIterator(_l, AL_START_HEAD);
        while (0 != (ln = listNext(li))) {
            ET_FactoryActorProcessEvent(FactoryActor, (etActorEvent_t*)ln->Value);

            ET_FactoryActorRecycleEvent(FactoryActor, (etActorEvent_t*)ln->Value);
            listDelNode(_l, ln);
        }
        listReleaseIterator(li);
        listRelease(_l);
    }
}

void ET_DeviceFactoryActorLooper(etDevice_t *device) {
    etFactoryActor_t *FactoryActor = device->FactoryActor;
    while(0 == device->LooperStop) {
        ET_DeviceFactoryActorLoopOnce(device);

        if (0 == listLength(FactoryActor->WaitingEventList)) {
            ET_DeviceWaitEventList(device);
        }
    }
}
