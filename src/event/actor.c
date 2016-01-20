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
    ETChannelActor *chanelActor = (ETChannelActor*)zmalloc(sizeof(ETChannelActor));
    memset(chanelActor, 0, sizeof(ETChannelActor));
    chanelActor->subscribers = listCreate();
    return chanelActor;
}

void ETFreeChannelActor(ETChannelActor *chanelActor) {
    sdsfree(chanelActor->key);
    listRelease(chanelActor->subscribers);
}

ETFactoryActor* ETNewFactoryActor(void) {
    ETFactoryActor *factoryActor = (ETFactoryActor*)zmalloc(sizeof(ETFactoryActor));
    memset(factoryActor, 0, sizeof(ETFactoryActor));

    factoryActor->free_actor_list = listCreate();

    factoryActor->running_event_list = listCreate();
    factoryActor->running_event_list->free = ETFreeActorEvent;

    factoryActor->waiting_event_list = listCreate();
    factoryActor->waiting_event_list->free = ETFreeActorEvent;

    factoryActor->channels = dictCreate(&ETKeyChannelDictType, NULL);

    return factoryActor;
}

void ETFreeFactoryActor(ETFactoryActor *factoryActor) {
    listIter *iter;
    listNode *node;
    iter = listGetIterator(factoryActor->free_actor_list, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {
        zfree(node->value);
    }
    listRelease(factoryActor->free_actor_list);

    listRelease(factoryActor->running_event_list);
    listRelease(factoryActor->waiting_event_list);

    zfree(factoryActor);
}

ETActor* ETFactoryActoNewActor(ETFactoryActor *factoryActor) {
    ETActor *actor;

    if (0 == factoryActor->free_actor_list) {
        for (int i = 0; i < 20; i++) {
            actor = (ETActor*)zmalloc(sizeof(ETActor));
            memset(actor, 0, sizeof(ETActor));

            factoryActor->free_actor_list = listAddNodeTail(factoryActor->free_actor_list, actor);
        }
    }

    listNode *ln = listFirst(factoryActor->free_actor_list);
    actor = (ETActor*)ln->value;
    listDelNode(factoryActor->free_actor_list, ln);

    memset(actor, 0, sizeof(ETActor));

    return actor;
}

void ETFactoryActorRecycleActor(ETFactoryActor *factoryActor, ETActor *actor) {
    listAddNodeTail(factoryActor->free_actor_list, actor);
}

void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent) {
    factoryActor->waiting_event_list = listAddNodeTail(factoryActor->waiting_event_list, actorEvent);
}

void ETFactoryActorProcessEvent(ETFactoryActor *factoryActor, ETActorEvent *event) {
    ETActor *actor;
    dictEntry *chanDe;
    ETChannelActor *channel;
    listIter *iterActor;
    listNode *nodeActor;

    if (0 != event->receiver) {
        actor = event->receiver;
        actor->proc(actor, event->mail_args, event->mail_argv);
    }

    if (0 != event->channel) {
        chanDe = dictFind(factoryActor->channels, event->channel);

        if (NULL != chanDe) {
            channel = (ETChannelActor*)dictGetVal(chanDe);
            iterActor = listGetIterator(channel->subscribers, AL_START_HEAD);
            while (NULL != (nodeActor = listNext(iterActor))) {
                actor = (ETActor*)nodeActor->value;
                actor->proc(actor, event->mail_args, event->mail_argv);
            }
        }
    }
}

void ETDeviceFactoryActorLooper(ETDevice *device) {
    ETFactoryActor *factoryActor = device->factory_actor;
    list *_l;
    listIter *iter;
    listNode *node;
    while(0 == device->looper_stop) {
        if (0 == factoryActor->running_event_list->len) {
            _l = factoryActor->running_event_list;
            factoryActor->running_event_list = factoryActor->waiting_event_list;
            factoryActor->waiting_event_list = _l;
        }

        iter = listGetIterator(factoryActor->running_event_list, AL_START_HEAD);
        while (NULL != (node = listNext(iter))) {
            ETFactoryActorProcessEvent(factoryActor, (ETActorEvent*)node->value);

            listDelNode(factoryActor->running_event_list, node);
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

        if (0 == factoryActor->waiting_event_list->len) {
            ETDeviceWaitEventList(device);
        }
    }
}
