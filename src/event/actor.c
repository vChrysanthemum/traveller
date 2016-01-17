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

ETActor* ETCreateActor(ETFactoryActor *factoryActor) {
    ETActor *actor = (ETActor*)zmalloc(sizeof(ETActor));
    factoryActor->actor_list = listAddNodeTail(factoryActor->actor_list, actor);
    return actor;
}

void ETFreeActor(void *_actor) {
    ETActor *actor = (ETActor*)_actor;
    zfree(actor);
}

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

    factoryActor->actor_list = listCreate();
    factoryActor->actor_list->free = ETFreeActor;

    factoryActor->running_event_list = listCreate();
    factoryActor->running_event_list->free = ETFreeActorEvent;

    factoryActor->waiting_event_list = listCreate();
    factoryActor->waiting_event_list->free = ETFreeActorEvent;

    factoryActor->channels = dictCreate(&ETKeyChannelDictType, NULL);

    return factoryActor;
}

void ETFreeFactoryActor(ETFactoryActor *factoryActor) {
    listRelease(factoryActor->running_event_list);
    listRelease(factoryActor->waiting_event_list);

    listRelease(factoryActor->actor_list);

    zfree(factoryActor);
}

void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent) {
    factoryActor->waiting_event_list = listAddNodeTail(factoryActor->waiting_event_list, actorEvent);
}

static void processActorEvent(ETFactoryActor *factoryActor, ETActorEvent *event) {
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

void* ETFactoryActorLoopEvent(ETFactoryActor *factoryActor) {

    if (0 == factoryActor->running_event_list->len) {
        if (0 == factoryActor->waiting_event_list->len) {
            return 0;
        }

        list *_l;
        _l = factoryActor->running_event_list;
        factoryActor->running_event_list = factoryActor->waiting_event_list;
        factoryActor->waiting_event_list = _l;
    }

    listIter *iter;
    listNode *node;
    iter = listGetIterator(factoryActor->running_event_list, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {
        processActorEvent(factoryActor, (ETActorEvent*)node->value);

        listDelNode(factoryActor->running_event_list, node);
    }

    return 0;
}
