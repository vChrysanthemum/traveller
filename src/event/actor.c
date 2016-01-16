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
    ETFreeChanelActor((ETChannelActor*)val);
}

ETChannelActor *ETNewChanelActor(void) {
    ETChannelActor *chanelActor = (ETChannelActor*)zmalloc(sizeof(ETChannelActor));
    memset(chanelActor, 0, sizeof(ETChannelActor));
    chanelActor->subscribers = listCreate();
    return chanelActor;
}

void ETFreeChanelActor(ETChannelActor *chanelActor) {
    sdsfree(chanelActor->key);
    listRelease(chanelActor->subscribers);
}

ETFactoryActor* ETNewFactoryActor(void) {
    ETFactoryActor *factoryActor = (ETFactoryActor*)zmalloc(sizeof(ETFactoryActor));
    memset(factoryActor, 0, sizeof(ETFactoryActor));

    factoryActor->actor_list = listCreate();
    factoryActor->actor_list->free = ETFreeActor;

    factoryActor->running_actor_event_list = listCreate();
    factoryActor->running_actor_event_list->free = ETFreeActorEvent;

    factoryActor->waiting_actor_event_list = listCreate();
    factoryActor->waiting_actor_event_list->free = ETFreeActorEvent;

    factoryActor->channels = dictCreate(&ETKeyChannelDictType, NULL);

    return factoryActor;
}

void ETFreeFactoryActor(ETFactoryActor *factoryActor) {
    listRelease(factoryActor->running_actor_event_list);
    listRelease(factoryActor->waiting_actor_event_list);

    listRelease(factoryActor->actor_list);

    zfree(factoryActor);
}

void ETFactoryActorAppendEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent) {
    factoryActor->waiting_actor_event_list = listAddNodeTail(factoryActor->waiting_actor_event_list, actorEvent);
}

void ETProcessActorEvent(ETFactoryActor *factoryActor) {
    list *_l;
    _l = factoryActor->running_actor_event_list;
    factoryActor->running_actor_event_list = factoryActor->waiting_actor_event_list;
    factoryActor->waiting_actor_event_list = _l;

    listIter *iter;
    listNode *node;
    iter = listGetIterator(factoryActor->running_actor_event_list, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {

        listDelNode(factoryActor->running_actor_event_list, node);
    }
}

