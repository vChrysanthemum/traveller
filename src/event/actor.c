#include "core/util.h"
#include "core/dict.h"
#include "event/event.h"

static void freeActorEvent(ETActorEvent *event);

void dictChannelDestructor(void *privdata, void *val) {
    DICT_NOTUSED(privdata);
    ETFreeChannelActor((ETChannelActor*)val);
}

dictType ETKeyChannelDictType = {
    dictStringHash,             /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictStringCompare,          /* key compare */
    dictStringDestructor,       /* key destructor */
    dictChannelDestructor       /* val destructor */
};

ETFactoryActor* ETNewFactoryActor(void) {
    ETFactoryActor *factoryActor = (ETFactoryActor*)zmalloc(sizeof(ETFactoryActor));
    memset(factoryActor, 0, sizeof(ETFactoryActor));

    factoryActor->freeActorEventList = listCreate();

    factoryActor->freeActorList = listCreate();

    factoryActor->runningEventList = listCreate();

    factoryActor->waitingEventList = listCreate();

    factoryActor->channels = dictCreate(&ETKeyChannelDictType, NULL);

    return factoryActor;
}

void ETFreeFactoryActor(ETFactoryActor *factoryActor) {
    listIter *li;
    listNode *ln;
    li = listGetIterator(factoryActor->freeActorEventList, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        freeActorEvent((ETActorEvent*)listNodeValue(ln));
    }
    listReleaseIterator(li);

    listRelease(factoryActor->freeActorList);
    li = listGetIterator(factoryActor->freeActorList, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        zfree(ln->value);
    }
    listReleaseIterator(li);
    listRelease(factoryActor->freeActorList);

    li = listGetIterator(factoryActor->runningEventList, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        freeActorEvent((ETActorEvent*)listNodeValue(ln));
    }
    listReleaseIterator(li);
    listRelease(factoryActor->runningEventList);

    li = listGetIterator(factoryActor->waitingEventList, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        freeActorEvent((ETActorEvent*)listNodeValue(ln));
    }
    listReleaseIterator(li);
    listRelease(factoryActor->waitingEventList);

    zfree(factoryActor);
}

static inline void freeActorEvent(ETActorEvent *actorEvent) {
    sdsfree(actorEvent->channel);
    zfree(actorEvent);
}

static inline void resetActorEvent(ETActorEvent *actorEvent) {
    sdsfree(actorEvent->channel);
}

ETActorEvent* ETFactoryActorNewEvent(ETFactoryActor *factoryActor) {
    ETActorEvent *actorEvent;

    if (0 == listLength(factoryActor->freeActorEventList)) {
        for (int i = 0; i < 20; i++) {
            actorEvent = (ETActorEvent*)zmalloc(sizeof(ETActorEvent));
            memset(actorEvent, 0, sizeof(ETActorEvent));

            factoryActor->freeActorEventList = listAddNodeTail(factoryActor->freeActorEventList, actorEvent);
        }
    }

    listNode *ln = listFirst(factoryActor->freeActorEventList);
    actorEvent = (ETActorEvent*)ln->value;
    listDelNode(factoryActor->freeActorEventList, ln);

    memset(actorEvent, 0, sizeof(ETActorEvent));

    return actorEvent;
}

void ETFactoryActorRecycleEvent(ETFactoryActor *factoryActor,  ETActorEvent *actorEvent) {
    if (listLength(factoryActor->freeActorEventList) > 200) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(factoryActor->freeActorEventList, AL_START_HEAD);
        for (int i = 0; i < 100 && NULL != (ln = listNext(li)); i++) {
            freeActorEvent((ETActorEvent*)listNodeValue(ln));
            listDelNode(factoryActor->freeActorEventList, ln);
        }
        listReleaseIterator(li);
    }

    resetActorEvent(actorEvent);
    factoryActor->freeActorEventList = listAddNodeTail(factoryActor->freeActorEventList, actorEvent);
}

void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent) {
    factoryActor->waitingEventList = listAddNodeTail(factoryActor->waitingEventList, actorEvent);
}

void ETFactoryActorProcessEvent(ETFactoryActor *factoryActor, ETActorEvent *event) {
    ETActor *actor;
    ETChannelActor *channel;
    listIter *liActor;
    listNode *lnActor;

    if (0 != event->receiver) {
        actor = event->receiver;
        actor->proc(actor, event->mailArgs, event->mailArgv);
    }

    if (0 != sdslen(event->channel)) {
        channel = (ETChannelActor*)dictFetchValue(factoryActor->channels, event->channel);

        if (0 != channel) {
            liActor = listGetIterator(channel->subscribers, AL_START_HEAD);
            while (NULL != (lnActor = listNext(liActor))) {
                actor = (ETActor*)lnActor->value;
                actor->proc(actor, event->mailArgs, event->mailArgv);
            }
            listReleaseIterator(liActor);
        }
    }
}

ETChannelActor *ETNewChannelActor(void) {
    ETChannelActor *channelActor = (ETChannelActor*)zmalloc(sizeof(ETChannelActor));
    memset(channelActor, 0, sizeof(ETChannelActor));
    channelActor->subscribers = listCreate();
    return channelActor;
}

void ETFreeChannelActor(ETChannelActor *channelActor) {
    zfree(channelActor->key);
    listRelease(channelActor->subscribers);
}

void ETFactoryActorAppendChannel(ETFactoryActor *factoryActor, ETChannelActor *channel) {
    dictAdd(factoryActor->channels, stringnew(channel->key), channel);
}

void ETFactoryActorRemoveChannel(ETFactoryActor *factoryActor, ETChannelActor *channel) {
    dictDelete(factoryActor->channels, channel->key);
}

void ETSubscribeChannel(ETActor *actor, ETChannelActor *channelActor) {
    if (0 == actor->channels) {
        actor->channels = listCreate();
    }
    actor->channels = listAddNodeTail(actor->channels, channelActor);
    channelActor->subscribers = listAddNodeTail(channelActor->subscribers, actor);
}

void ETUnSubscribeChannel(ETActor *actor, ETChannelActor *channelActor) {
    listIter *li;
    listNode *ln;

    li = listGetIterator(actor->channels, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        if ((void*)channelActor == listNodeValue(ln)) {
            listDelNode(actor->channels, ln);
            break;
        }
    }
    listReleaseIterator(li);

    li = listGetIterator(channelActor->subscribers, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        if ((void*)actor == listNodeValue(ln)) {
            listDelNode(channelActor->subscribers, ln);
            break;
        }
    }
    listReleaseIterator(li);
}

static inline void freeActorChannels(ETActor *actor) {
    if (0 != actor->channels) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(actor->channels, AL_START_HEAD);
        while (NULL != (ln = listNext(li))) {
            ETUnSubscribeChannel(actor, (ETChannelActor*)listNodeValue(ln));
        }
        listReleaseIterator(li);
        listRelease(actor->channels);
    }
}

static inline void freeActor(ETActor *actor) {
    freeActorChannels(actor);
    zfree(actor);
}

static inline void resetActor(ETActor *actor) {
    freeActorChannels(actor);
}

ETActor* ETFactoryActorNewActor(ETFactoryActor *factoryActor) {
    ETActor *actor;

    if (0 == listLength(factoryActor->freeActorList)) {
        for (int i = 0; i < 20; i++) {
            actor = (ETActor*)zmalloc(sizeof(ETActor));
            memset(actor, 0, sizeof(ETActor));

            factoryActor->freeActorList = listAddNodeTail(factoryActor->freeActorList, actor);
        }
    }

    listNode *ln = listFirst(factoryActor->freeActorList);
    actor = (ETActor*)ln->value;
    listDelNode(factoryActor->freeActorList, ln);

    memset(actor, 0, sizeof(ETActor));

    return actor;
}

void ETFactoryActorRecycleActor(ETFactoryActor *factoryActor, ETActor *actor) {
    if (listLength(factoryActor->freeActorList) > 200) {
        listIter *li;
        listNode *ln;
        li = listGetIterator(factoryActor->freeActorList, AL_START_HEAD);
        for (int i = 0; i < 100 && NULL != (ln = listNext(li)); i++) {
            freeActor((ETActor*)listNodeValue(ln));
            listDelNode(factoryActor->freeActorList, ln);
        }
        listReleaseIterator(li);
    }

    resetActor(actor);
    factoryActor->freeActorList = listAddNodeTail(factoryActor->freeActorList, actor);
}

void ETDeviceFactoryActorLoopOnce(ETDevice *device) {
    ETFactoryActor *factoryActor = device->factoryActor;
    list *_l;
    listIter *li;
    listNode *ln;

    if (0 == listLength(factoryActor->runningEventList)) {
        _l = factoryActor->runningEventList;
        factoryActor->runningEventList = factoryActor->waitingEventList;
        factoryActor->waitingEventList = _l;
    }

    li = listGetIterator(factoryActor->runningEventList, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        ETFactoryActorProcessEvent(factoryActor, (ETActorEvent*)ln->value);

        ETFactoryActorRecycleEvent(factoryActor, (ETActorEvent*)ln->value);
        listDelNode(factoryActor->runningEventList, ln);
    }
    listReleaseIterator(li);

    _l = ETDevicePopEventList(device);
    if (0 != _l) {
        li = listGetIterator(_l, AL_START_HEAD);
        while (NULL != (ln = listNext(li))) {
            ETFactoryActorProcessEvent(factoryActor, (ETActorEvent*)ln->value);

            ETFactoryActorRecycleEvent(factoryActor, (ETActorEvent*)ln->value);
            listDelNode(_l, ln);
        }
        listReleaseIterator(li);
        listRelease(_l);
    }
}

void ETDeviceFactoryActorLooper(ETDevice *device) {
    ETFactoryActor *factoryActor = device->factoryActor;
    while(0 == device->looperStop) {
        ETDeviceFactoryActorLoopOnce(device);

        if (0 == listLength(factoryActor->waitingEventList)) {
            ETDeviceWaitEventList(device);
        }
    }
}
