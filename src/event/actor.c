#include "core/util.h"
#include "core/dict.h"
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
    etFactoryActor_t *factoryActor = (etFactoryActor_t*)zmalloc(sizeof(etFactoryActor_t));
    memset(factoryActor, 0, sizeof(etFactoryActor_t));

    factoryActor->actorEventPool = listCreate();

    factoryActor->actorPool = listCreate();

    factoryActor->runningEventList = listCreate();

    factoryActor->waitingEventList = listCreate();

    factoryActor->channels = dictCreate(&ETKeyChannelDictType, NULL);

    return factoryActor;
}

void ET_FreeFactoryActor(etFactoryActor_t *factoryActor) {
    listIter *li;
    listNode *ln;
    li = listGetIterator(factoryActor->actorEventPool, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        freeActorEvent((etActorEvent_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);

    listRelease(factoryActor->actorPool);
    li = listGetIterator(factoryActor->actorPool, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        zfree(ln->value);
    }
    listReleaseIterator(li);
    listRelease(factoryActor->actorPool);

    li = listGetIterator(factoryActor->runningEventList, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        freeActorEvent((etActorEvent_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);
    listRelease(factoryActor->runningEventList);

    li = listGetIterator(factoryActor->waitingEventList, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        freeActorEvent((etActorEvent_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);
    listRelease(factoryActor->waitingEventList);

    zfree(factoryActor);
}

static inline void freeActorEvent(etActorEvent_t *actorEvent) {
    sdsfree(actorEvent->channel);
    zfree(actorEvent);
}

static inline void resetActorEvent(etActorEvent_t *actorEvent) {
    sdsfree(actorEvent->channel);
}

etActorEvent_t* ET_FactoryActorNewEvent(etFactoryActor_t *factoryActor) {
    etActorEvent_t *actorEvent;

    if (0 == listLength(factoryActor->actorEventPool)) {
        for (int i = 0; i < 20; i++) {
            actorEvent = (etActorEvent_t*)zmalloc(sizeof(etActorEvent_t));
            memset(actorEvent, 0, sizeof(etActorEvent_t));

            factoryActor->actorEventPool = listAddNodeTail(factoryActor->actorEventPool, actorEvent);
        }
    }

    listNode *ln = listFirst(factoryActor->actorEventPool);
    actorEvent = (etActorEvent_t*)ln->value;
    listDelNode(factoryActor->actorEventPool, ln);

    memset(actorEvent, 0, sizeof(etActorEvent_t));

    return actorEvent;
}

void ET_FactoryActorRecycleEvent(etFactoryActor_t *factoryActor,  etActorEvent_t *actorEvent) {
    if (listLength(factoryActor->actorEventPool) > 200) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(factoryActor->actorEventPool, AL_START_HEAD);
        for (int i = 0; i < 100 && 0 != (ln = listNext(li)); i++) {
            freeActorEvent((etActorEvent_t*)listNodeValue(ln));
            listDelNode(factoryActor->actorEventPool, ln);
        }
        listReleaseIterator(li);
    }

    resetActorEvent(actorEvent);
    factoryActor->actorEventPool = listAddNodeTail(factoryActor->actorEventPool, actorEvent);
}

void ET_FactoryActorAppendEvent(etFactoryActor_t *factoryActor, etActorEvent_t *actorEvent) {
    factoryActor->waitingEventList = listAddNodeTail(factoryActor->waitingEventList, actorEvent);
}

void ET_FactoryActorProcessEvent(etFactoryActor_t *factoryActor, etActorEvent_t *event) {
    etActor_t *actor;
    etChannelActor_t *channel;
    listIter *liActor;
    listNode *lnActor;

    if (0 != event->receiver) {
        actor = event->receiver;
        actor->proc(actor, event->mailArgs, event->mailArgv);
    }

    if (0 != sdslen(event->channel)) {
        channel = (etChannelActor_t*)dictFetchValue(factoryActor->channels, event->channel);

        if (0 != channel) {
            liActor = listGetIterator(channel->subscribers, AL_START_HEAD);
            while (0 != (lnActor = listNext(liActor))) {
                actor = (etActor_t*)lnActor->value;
                actor->proc(actor, event->mailArgs, event->mailArgv);
            }
            listReleaseIterator(liActor);
        }
    }
}

etChannelActor_t *ET_NewChannelActor(void) {
    etChannelActor_t *channelActor = (etChannelActor_t*)zmalloc(sizeof(etChannelActor_t));
    memset(channelActor, 0, sizeof(etChannelActor_t));
    channelActor->subscribers = listCreate();
    return channelActor;
}

void ET_FreeChannelActor(etChannelActor_t *channelActor) {
    zfree(channelActor->key);
    listRelease(channelActor->subscribers);
}

void ET_FactoryActorAppendChannel(etFactoryActor_t *factoryActor, etChannelActor_t *channel) {
    dictAdd(factoryActor->channels, stringnew(channel->key), channel);
}

void ET_FactoryActorRemoveChannel(etFactoryActor_t *factoryActor, etChannelActor_t *channel) {
    dictDelete(factoryActor->channels, channel->key);
}

void ET_SubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor) {
    if (0 == actor->channels) {
        actor->channels = listCreate();
    }
    actor->channels = listAddNodeTail(actor->channels, channelActor);
    channelActor->subscribers = listAddNodeTail(channelActor->subscribers, actor);
}

void ET_UnSubscribeChannel(etActor_t *actor, etChannelActor_t *channelActor) {
    listIter *li;
    listNode *ln;

    li = listGetIterator(actor->channels, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        if ((void*)channelActor == listNodeValue(ln)) {
            listDelNode(actor->channels, ln);
            break;
        }
    }
    listReleaseIterator(li);

    li = listGetIterator(channelActor->subscribers, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        if ((void*)actor == listNodeValue(ln)) {
            listDelNode(channelActor->subscribers, ln);
            break;
        }
    }
    listReleaseIterator(li);
}

static inline void freeActorChannels(etActor_t *actor) {
    if (0 != actor->channels) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(actor->channels, AL_START_HEAD);
        while (0 != (ln = listNext(li))) {
            ET_UnSubscribeChannel(actor, (etChannelActor_t*)listNodeValue(ln));
        }
        listReleaseIterator(li);
        listRelease(actor->channels);
    }
}

static inline void freeActor(etActor_t *actor) {
    freeActorChannels(actor);
    zfree(actor);
}

static inline void resetActor(etActor_t *actor) {
    freeActorChannels(actor);
}

etActor_t* ET_FactoryActorNewActor(etFactoryActor_t *factoryActor) {
    etActor_t *actor;

    if (0 == listLength(factoryActor->actorPool)) {
        for (int i = 0; i < 20; i++) {
            actor = (etActor_t*)zmalloc(sizeof(etActor_t));
            memset(actor, 0, sizeof(etActor_t));

            factoryActor->actorPool = listAddNodeTail(factoryActor->actorPool, actor);
        }
    }

    listNode *ln = listFirst(factoryActor->actorPool);
    actor = (etActor_t*)ln->value;
    listDelNode(factoryActor->actorPool, ln);

    memset(actor, 0, sizeof(etActor_t));

    return actor;
}

void ET_FactoryActorRecycleActor(etFactoryActor_t *factoryActor, etActor_t *actor) {
    if (listLength(factoryActor->actorPool) > 200) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(factoryActor->actorPool, AL_START_HEAD);
        for (int i = 0; i < 100 && 0 != (ln = listNext(li)); i++) {
            freeActor((etActor_t*)listNodeValue(ln));
            listDelNode(factoryActor->actorPool, ln);
        }
        listReleaseIterator(li);
    }

    resetActor(actor);
    factoryActor->actorPool = listAddNodeTail(factoryActor->actorPool, actor);
}

void ET_DeviceFactoryActorLoopOnce(etDevice_t *device) {
    etFactoryActor_t *factoryActor = device->factoryActor;
    list *_l;
    listIter *li;
    listNode *ln;

    if (0 == listLength(factoryActor->runningEventList)) {
        _l = factoryActor->runningEventList;
        factoryActor->runningEventList = factoryActor->waitingEventList;
        factoryActor->waitingEventList = _l;
    }

    li = listGetIterator(factoryActor->runningEventList, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        ET_FactoryActorProcessEvent(factoryActor, (etActorEvent_t*)ln->value);

        ET_FactoryActorRecycleEvent(factoryActor, (etActorEvent_t*)ln->value);
        listDelNode(factoryActor->runningEventList, ln);
    }
    listReleaseIterator(li);

    _l = ET_DevicePopEventList(device);
    if (0 != _l) {
        li = listGetIterator(_l, AL_START_HEAD);
        while (0 != (ln = listNext(li))) {
            ET_FactoryActorProcessEvent(factoryActor, (etActorEvent_t*)ln->value);

            ET_FactoryActorRecycleEvent(factoryActor, (etActorEvent_t*)ln->value);
            listDelNode(_l, ln);
        }
        listReleaseIterator(li);
        listRelease(_l);
    }
}

void ET_DeviceFactoryActorLooper(etDevice_t *device) {
    etFactoryActor_t *factoryActor = device->factoryActor;
    while(0 == device->looperStop) {
        ET_DeviceFactoryActorLoopOnce(device);

        if (0 == listLength(factoryActor->waitingEventList)) {
            ET_DeviceWaitEventList(device);
        }
    }
}
