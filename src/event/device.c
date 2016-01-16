#include "event/event.h"

void ETFreeDeviceJob(void* _job) {
    ETDeviceJob *job = (ETDeviceJob*)_job;
    zfree(job);
}

ETDevice* ETNewDevice(void) {
    ETDevice *device = (ETDevice*)zmalloc(sizeof(ETDevice));
    memset(device, 0, sizeof(ETDevice));

    device->jobs = listCreate();
    device->jobs->free = ETFreeDeviceJob;

    device->factory_actor = ETNewFactoryActor();
    pthread_mutex_init(&device->mutex, NULL);

    return device;
}

void ETFreeDevice(ETDevice *device) {
    ETFreeFactoryActor(device->factory_actor);
    pthread_mutex_destroy(&device->mutex);
    zfree(device);
}

static void* deviceStartJob(void *_param) {
    ETDeviceStartJobParam *param = (ETDeviceStartJobParam*)_param;
    ETDevice *device = param->device;
    ETDeviceJob *job = param->job;

    pthread_create(&job->ntid, job->arg, job->start_routine, job->arg);

    pthread_join(job->ntid, &job->exit_status);

    listDelNode(device->jobs, listIndex(device->jobs, job->index));

    return NULL;
}

pthread_t ETDeviceStartJob(ETDevice *device, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg) {

    ETDeviceJob *job = (ETDeviceJob*)zmalloc(sizeof(ETDeviceJob));
    memset(job, 0, sizeof(ETDeviceJob));

    job->pthread_attr = attr;
    job->arg = arg;
    job->start_routine = start_routine;

    pthread_mutex_lock(&device->mutex);
    device->jobs = listAddNodeTail(device->jobs, job);
    job->index = device->jobs->len - 1;
    pthread_mutex_unlock(&device->mutex);

    ETDeviceStartJobParam param = {device, job};

    pthread_t ntid;
    pthread_create(&ntid, NULL, deviceStartJob, &param);
    return ntid;
}

void ETDeviceAppendActorEvent(ETDevice *device, ETActorEvent *actorEvent) {
}
