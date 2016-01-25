#include "core/util.h"
#include "core/dict.h"
#include "event/event.h"

dictType ETKeyChannelDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    dictSdsDestructor,          /* key destructor */
    dictChannelDestructor       /* val destructor */
};

ETActorEvent *ETNewActorEvent(void) {
    ETActorEvent *actorEvent = (ETActorEvent*)zmalloc(sizeof(ETActorEvent));
    memset(actorEvent, 0, sizeof(ETActorEvent));
    actorEvent->channel = sdsempty();

    return actorEvent;
}

void ETFreeActorEvent(void *_actorEvent) {
    ETActorEvent *actorEvent = (ETActorEvent*)_actorEvent;
    sdsfree(actorEvent->channel);
    zfree(actorEvent);
}

void dictChannelDestructor(void *privdata, void *val) {
    DICT_NOTUSED(privdata);
    ETFreeChannelActor((ETChannelActor*)val);
}

ETChannelActor *ETNewChannelActor(void) {
    ETChannelActor *channelActor = (ETChannelActor*)zmalloc(sizeof(ETChannelActor));
    memset(channelActor, 0, sizeof(ETChannelActor));
    channelActor->key = sdsempty();
    channelActor->subscribers = listCreate();
    return channelActor;
}

void ETFreeChannelActor(ETChannelActor *channelActor) {
    sdsfree(channelActor->key);
    listRelease(channelActor->subscribers);
}

ETFactoryActor* ETNewFactoryActor(void) {
    ETFactoryActor *factoryActor = (ETFactoryActor*)zmalloc(sizeof(ETFactoryActor));
    memset(factoryActor, 0, sizeof(ETFactoryActor));

    factoryActor->freeActorList = listCreate();

    factoryActor->runningEventList = listCreate();
    factoryActor->runningEventList->free = ETFreeActorEvent;

    factoryActor->waitingEventList = listCreate();
    factoryActor->waitingEventList->free = ETFreeActorEvent;

    factoryActor->channels = dictCreate(&ETKeyChannelDictType, NULL);

    return factoryActor;
}

void ETFreeFactoryActor(ETFactoryActor *factoryActor) {
    listIter *iter;
    listNode *node;
    iter = listGetIterator(factoryActor->freeActorList, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {
        zfree(node->value);
    }
    listRelease(factoryActor->freeActorList);

    listRelease(factoryActor->runningEventList);
    listRelease(factoryActor->waitingEventList);

    zfree(factoryActor);
}

ETActor* ETFactoryActoNewActor(ETFactoryActor *factoryActor) {
    ETActor *actor;

    if (0 == factoryActor->freeActorList) {
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
    listAddNodeTail(factoryActor->freeActorList, actor);
}

void ETFactoryActorAppendChannel(ETFactoryActor *factoryActor, ETChannelActor *channel) {
    dictAdd(factoryActor->channels, channel->key, channel);
}

void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent) {
    factoryActor->waitingEventList = listAddNodeTail(factoryActor->waitingEventList, actorEvent);
}

void ETFactoryActorProcessEvent(ETFactoryActor *factoryActor, ETActorEvent *event) {
    ETActor *actor;
    dictEntry *chanDe;
    ETChannelActor *channel;
    listIter *iterActor;
    listNode *nodeActor;

    if (0 != event->receiver) {
        actor = event->receiver;
        actor->proc(actor, event->mailArgs, event->mailArgv);
    }

    if (0 != sdslen(event->channel)) {
        chanDe = dictFind(factoryActor->channels, event->channel);

        if (NULL != chanDe) {
            channel = (ETChannelActor*)dictGetVal(chanDe);
            iterActor = listGetIterator(channel->subscribers, AL_START_HEAD);
            while (NULL != (nodeActor = listNext(iterActor))) {
                actor = (ETActor*)nodeActor->value;
                actor->proc(actor, event->mailArgs, event->mailArgv);
            }
        }
    }
    TrvLogD("hi");
}

void ETDeviceFactoryActorLoopOnce(ETDevice *device) {
    ETFactoryActor *factoryActor = device->factoryActory;
    list *_l;
    listIter *iter;
    listNode *node;
    if (0 == factoryActor->runningEventList->len) {
        _l = factoryActor->runningEventList;
        factoryActor->runningEventList = factoryActor->waitingEventList;
        factoryActor->waitingEventList = _l;
    }

    iter = listGetIterator(factoryActor->runningEventList, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {
        ETFactoryActorProcessEvent(factoryActor, (ETActorEvent*)node->value);

        listDelNode(factoryActor->runningEventList, node);
    }

    _l = ETDevicePopEventList(device);
    if (0 != _l) {
        iter = listGetIterator(_l, AL_START_HEAD);
        while (NULL != (node = listNext(iter))) {
            ETFactoryActorProcessEvent(factoryActor, (ETActorEvent*)node->value);

            listDelNode(_l, node);
        }
        listRelease(_l);
    }
}

void ETDeviceFactoryActorLooper(ETDevice *device) {
    ETFactoryActor *factoryActor = device->factoryActory;
    while(0 == device->looperStop) {
        ETDeviceFactoryActorLoopOnce(device);

        if (0 == factoryActor->waitingEventList->len) {
            ETDeviceWaitEventList(device);
        }
    }
}
