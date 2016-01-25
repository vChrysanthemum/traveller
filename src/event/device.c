#include <unistd.h>
#include "event/event.h"

void ETFreeDeviceJob(void* _job) {
    ETDeviceJob *job = (ETDeviceJob*)_job;
    zfree(job);
}

ETDevice* ETNewDevice(ETDeviceLooper looper, void *arg) {
    ETDevice *device = (ETDevice*)zmalloc(sizeof(ETDevice));
    memset(device, 0, sizeof(ETDevice));
    
    device->jobs = listCreate();
    device->jobs->free = ETFreeDeviceJob;

    device->factoryActory = ETNewFactoryActor();
    pthread_mutex_init(&device->jobMutex, 0);
    pthread_mutex_init(&device->actorMutex, 0);
    pthread_mutex_init(&device->eventMutex, 0);
    pthread_mutex_init(&device->eventWaitMutex, 0);
    pthread_cond_init(&device->eventWaitCond, 0);

    device->waitingEventList = listCreate();

    device->looper = looper;
    device->looperArg = arg;

    return device;
}

void ETFreeDevice(ETDevice *device) {
    ETFreeFactoryActor(device->factoryActory);
    pthread_mutex_destroy(&device->jobMutex);
    pthread_mutex_destroy(&device->actorMutex);
    pthread_mutex_destroy(&device->eventMutex);
    pthread_mutex_destroy(&device->eventWaitMutex);
    pthread_cond_destroy(&device->eventWaitCond);
    zfree(device);
}

void ETDeviceStart(ETDevice *device) {
    if (0 != device->looper) {
        pthread_create(&device->looperNtid, 0, device->looper, device->looperArg);
    }
}

static void* deviceStartJob(void *_param) {
    ETDeviceStartJobParam *param = (ETDeviceStartJobParam*)_param;
    ETDevice *device = param->device;
    ETDeviceJob *job = param->job;

    pthread_create(&job->ntid, job->pthreadAttr, job->startRoutine, job->arg);

    pthread_join(job->ntid, &job->exitStatus);

    listDelNode(device->jobs, listIndex(device->jobs, job->index));

    return 0;
}

pthread_t ETDeviceStartJob(ETDevice *device, const pthread_attr_t *attr,
        void *(*startRoutine) (void *), void *arg) {

    ETDeviceJob *job = (ETDeviceJob*)zmalloc(sizeof(ETDeviceJob));
    memset(job, 0, sizeof(ETDeviceJob));

    job->pthreadAttr = attr;
    job->arg = arg;
    job->startRoutine = startRoutine;

    pthread_mutex_lock(&device->jobMutex);
    device->jobs = listAddNodeTail(device->jobs, job);
    job->index = device->jobs->len - 1;
    pthread_mutex_unlock(&device->jobMutex);

    ETDeviceStartJobParam param = {device, job};

    pthread_t ntid;
    pthread_create(&ntid, 0, deviceStartJob, &param);
    return ntid;
}

void ETDeviceAppendEvent(ETDevice *device, ETActorEvent *event) {
    pthread_mutex_lock(&device->eventWaitMutex);

    pthread_mutex_lock(&device->eventMutex);
    listAddNodeTail(device->waitingEventList, event);

    pthread_mutex_unlock(&device->eventMutex);

    pthread_cond_signal(&device->eventWaitCond);
    pthread_mutex_unlock(&device->eventWaitMutex);
}

list* ETDevicePopEventList(ETDevice *device) {
    list* _l;

    if (0 == device->waitingEventList->len) {
        return 0;
    }

    pthread_mutex_lock(&device->eventMutex);
    _l = device->waitingEventList;
    device->waitingEventList = listCreate();
    pthread_mutex_unlock(&device->eventMutex);

    return _l;
}

void ETDeviceWaitEventList(ETDevice *device) {
    pthread_mutex_lock(&device->eventWaitMutex);
    if (0 == device->waitingEventList->len) {
        pthread_cond_wait(&device->eventWaitCond, &device->eventWaitMutex);
    }
    pthread_mutex_unlock(&device->eventWaitMutex);

    if (device->waitingEventList->len < 20) {
        usleep(300);
    }
}
