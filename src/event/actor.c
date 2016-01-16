#include <pthread.h>
#include "core/util.h"
#include "core/dict.h"
#include "event/ae.h"

dictType ETKeyChannelDictType = {
    dictSdsHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCompare,          /* key compare */
    dictSdsDestructor,          /* key destructor */
    dictChannelDestructor       /* val destructor */
};

ETActor* ETNewActor(void) {
    ETActor *actor = (ETActor*)zmalloc(sizeof(ETActor));
    return actor;
}

void ETFreeActor(void *_actor) {
    ETActor *actor = (ETActor*)_actor;
    zfree(actor);
}

ETActorEvent *ETNewActorEvent(void) {
    ETActorEvent *actorEvent = (ETActorEvent*)zmalloc(sizeof(ETActorEvent));
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
    chanelActor->subscribers = listCreate();
    return chanelActor;
}

void ETFreeChanelActor(ETChannelActor *chanelActor) {
    sdsfree(chanelActor->key);
    listRelease(chanelActor->subscribers);
}

ETFactoryActor* ETCreateFactoryActor(void) {
    ETFactoryActor *factoryActor = (ETFactoryActor*)zmalloc(sizeof(ETFactoryActor));

    factoryActor->actor_list = listCreate();
    factoryActor->actor_list->free = ETFreeActor;

    pthread_mutex_init(&factoryActor->actor_mutex, NULL);

    factoryActor->running_actor_event_list = listCreate();
    factoryActor->running_actor_event_list->free = ETFreeActorEvent;

    factoryActor->waiting_actor_event_list = listCreate();
    factoryActor->waiting_actor_event_list->free = ETFreeActorEvent;

    pthread_mutex_init(&factoryActor->actor_event_mutex, NULL);

    factoryActor->channels = dictCreate(&ETKeyChannelDictType, NULL);

    return factoryActor;
}

void ETFreeFactoryActor(ETFactoryActor *factoryActor) {
    listRelease(factoryActor->running_actor_event_list);
    listRelease(factoryActor->waiting_actor_event_list);
    pthread_mutex_destroy(&factoryActor->actor_event_mutex);

    listRelease(factoryActor->actor_list);
    pthread_mutex_destroy(&factoryActor->actor_mutex);

    zfree(factoryActor);
}

void ETHostingActor(ETFactoryActor *factoryActor, ETActor *actor) {
    factoryActor->actor_list = listAddNodeTail(factoryActor->actor_list, actor);
}

void ETHostingActorEvent(ETFactoryActor *factoryActor, ETActorEvent *actorEvent) {
    pthread_mutex_lock(&factoryActor->actor_event_mutex);
    factoryActor->waiting_actor_event_list = listAddNodeTail(factoryActor->waiting_actor_event_list, actorEvent);
    pthread_mutex_unlock(&factoryActor->actor_event_mutex);
}

void ETProcessActorEvent(ETFactoryActor *factoryActor) {
    list *_l;
    pthread_mutex_lock(&factoryActor->actor_event_mutex);
    _l = factoryActor->running_actor_event_list;
    factoryActor->running_actor_event_list = factoryActor->waiting_actor_event_list;
    factoryActor->waiting_actor_event_list = _l;
    pthread_mutex_unlock(&factoryActor->actor_event_mutex);

    listIter *iter;
    listNode *node;
    iter = listGetIterator(factoryActor->running_actor_event_list, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {

        listDelNode(factoryActor->running_actor_event_list, node);
    }
}

