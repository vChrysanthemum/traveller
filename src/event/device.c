#include <unistd.h>
#include <pthread.h>

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/zmalloc.h"
#include "core/extern.h"

#include "event/event.h"

void ET_FreeDeviceJob(void* _job) {
    etDeviceJob_t *job = (etDeviceJob_t*)_job;
    zfree(job);
}

etDevice_t* ET_NewDevice(etDevice_tLooper looper, void *arg) {
    etDevice_t *device = (etDevice_t*)zmalloc(sizeof(etDevice_t));
    memset(device, 0, sizeof(etDevice_t));
    
    device->Jobs = listCreate();
    device->Jobs->free = ET_FreeDeviceJob;

    device->FactoryActor = ET_NewFactoryActor();
    pthread_mutex_init(&device->jobMutex, 0);
    pthread_mutex_init(&device->actorMutex, 0);
    pthread_mutex_init(&device->eventMutex, 0);
    pthread_mutex_init(&device->eventWaitMutex, 0);
    pthread_cond_init(&device->eventWaitCond, 0);

    device->WaitingEventList = listCreate();

    device->Looper = looper;
    device->LooperArg = arg;

    return device;
}

void ET_FreeDevice(etDevice_t *device) {
    ET_FreeFactoryActor(device->FactoryActor);
    pthread_mutex_destroy(&device->jobMutex);
    pthread_mutex_destroy(&device->actorMutex);
    pthread_mutex_destroy(&device->eventMutex);
    pthread_mutex_destroy(&device->eventWaitMutex);
    pthread_cond_destroy(&device->eventWaitCond);
    zfree(device);
}

void ET_StartDevice(etDevice_t *device) {
    if (0 != device->Looper) {
        pthread_create(&device->LooperNtid, 0, device->Looper, device->LooperArg);
    }
}

static void* deviceStartJob(void *_param) {
    etDeviceStartJobParam_t *param = (etDeviceStartJobParam_t*)_param;
    etDevice_t *device = param->Device;
    etDeviceJob_t *job = param->Job;

    pthread_create(&job->ntid, job->pthreadAttr, job->StartRoutine, job->Arg);

    pthread_join(job->ntid, &job->ExitStatus);

    listDelNode(device->Jobs, listIndex(device->Jobs, job->Index));

    return 0;
}

pthread_t ET_DeviceStartJob(etDevice_t *device, const pthread_attr_t *attr,
        void *(*startRoutine) (void *), void *arg) {

    etDeviceJob_t *job = (etDeviceJob_t*)zmalloc(sizeof(etDeviceJob_t));
    memset(job, 0, sizeof(etDeviceJob_t));

    job->pthreadAttr = attr;
    job->Arg = arg;
    job->StartRoutine = startRoutine;

    pthread_mutex_lock(&device->jobMutex);
    device->Jobs = listAddNodeTail(device->Jobs, job);
    job->Index = listLength(device->Jobs) - 1;
    pthread_mutex_unlock(&device->jobMutex);

    etDeviceStartJobParam_t param = {device, job};

    pthread_t ntid;
    pthread_create(&ntid, 0, deviceStartJob, &param);
    return ntid;
}

void ET_DeviceAppendEvent(etDevice_t *device, etActorEvent_t *event) {
    pthread_mutex_lock(&device->eventWaitMutex);

    pthread_mutex_lock(&device->eventMutex);
    device->WaitingEventList = listAddNodeTail(device->WaitingEventList, event);

    pthread_mutex_unlock(&device->eventMutex);

    pthread_cond_signal(&device->eventWaitCond);
    pthread_mutex_unlock(&device->eventWaitMutex);
}

list* ET_DevicePopEventList(etDevice_t *device) {
    list* _l;

    if (0 == listLength(device->WaitingEventList)) {
        return 0;
    }

    pthread_mutex_lock(&device->eventMutex);
    _l = device->WaitingEventList;
    device->WaitingEventList = listCreate();
    pthread_mutex_unlock(&device->eventMutex);

    return _l;
}

void ET_DeviceWaitEventList(etDevice_t *device) {
    pthread_mutex_lock(&device->eventWaitMutex);
    if (0 == listLength(device->WaitingEventList)) {
        pthread_cond_wait(&device->eventWaitCond, &device->eventWaitMutex);
    }
    pthread_mutex_unlock(&device->eventWaitMutex);

    if (listLength(device->WaitingEventList) < 20) {
        usleep(300);
    }
    C_UtilLogI("%lu", listLength(device->WaitingEventList));
}
