#include <unistd.h>
#include "event/event.h"

void ETFreeDeviceJob(void* _job) {
    ETDeviceJob *job = (ETDeviceJob*)_job;
    zfree(job);
}

ETDevice* ETNewDevice(ETDeviceLooper looper, void *arg) {
    ETDevice *device = (ETDevice*)zmalloc(sizeof(ETDevice));
    memset(device, 0, sizeof(ETDevice));
    
    device->stage = ETDEVICE_STAGE_INIT;

    device->jobs = listCreate();
    device->jobs->free = ETFreeDeviceJob;

    device->factory_actor = ETNewFactoryActor();
    pthread_mutex_init(&device->job_mutex, 0);
    pthread_mutex_init(&device->actor_mutex, 0);
    pthread_mutex_init(&device->event_mutex, 0);
    pthread_mutex_init(&device->event_wait_mutex, 0);
    pthread_cond_init(&device->event_wait_cond, 0);

    device->waiting_event_list = listCreate();

    device->looper = looper;
    device->looper_arg = arg;

    return device;
}

void ETFreeDevice(ETDevice *device) {
    ETFreeFactoryActor(device->factory_actor);
    pthread_mutex_destroy(&device->job_mutex);
    pthread_mutex_destroy(&device->actor_mutex);
    pthread_mutex_destroy(&device->event_mutex);
    pthread_mutex_destroy(&device->event_wait_mutex);
    pthread_cond_destroy(&device->event_wait_cond);
    zfree(device);
}

void ETDeviceStart(ETDevice *device) {
    device->stage = ETDEVICE_STAGE_RUNNING;
    if (0 != device->looper) {
        pthread_create(&device->looper_ntid, 0, device->looper, device->looper_arg);
    }
}

static void* deviceStartJob(void *_param) {
    ETDeviceStartJobParam *param = (ETDeviceStartJobParam*)_param;
    ETDevice *device = param->device;
    ETDeviceJob *job = param->job;

    pthread_create(&job->ntid, job->pthread_attr, job->start_routine, job->arg);

    pthread_join(job->ntid, &job->exit_status);

    listDelNode(device->jobs, listIndex(device->jobs, job->index));

    return 0;
}

pthread_t ETDeviceStartJob(ETDevice *device, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg) {

    ETDeviceJob *job = (ETDeviceJob*)zmalloc(sizeof(ETDeviceJob));
    memset(job, 0, sizeof(ETDeviceJob));

    job->pthread_attr = attr;
    job->arg = arg;
    job->start_routine = start_routine;

    pthread_mutex_lock(&device->job_mutex);
    device->jobs = listAddNodeTail(device->jobs, job);
    job->index = device->jobs->len - 1;
    pthread_mutex_unlock(&device->job_mutex);

    ETDeviceStartJobParam param = {device, job};

    pthread_t ntid;
    pthread_create(&ntid, 0, deviceStartJob, &param);
    return ntid;
}

void ETDeviceAppendEvent(ETDevice *device, ETActorEvent *event) {
    pthread_mutex_lock(&device->event_wait_mutex);

    pthread_mutex_lock(&device->event_mutex);
    listAddNodeTail(device->waiting_event_list, event);

    pthread_mutex_unlock(&device->event_mutex);

    pthread_cond_signal(&device->event_wait_cond);
    pthread_mutex_unlock(&device->event_wait_mutex);
}

list* ETDevicePopEventList(ETDevice *device) {
    list* _l;

    if (0 == device->waiting_event_list->len) {
        return 0;
    }

    pthread_mutex_lock(&device->event_mutex);
    _l = device->waiting_event_list;
    device->waiting_event_list = listCreate();
    pthread_mutex_unlock(&device->event_mutex);

    return _l;
}

void ETDeviceWaitEventList(ETDevice *device) {
    pthread_mutex_lock(&device->event_wait_mutex);
    if (0 == device->waiting_event_list->len) {
        pthread_cond_wait(&device->event_wait_cond, &device->event_wait_mutex);
    }
    pthread_mutex_unlock(&device->event_wait_mutex);

    if (device->waiting_event_list->len < 20) {
        usleep(300);
    }
}
