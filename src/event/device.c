#include "event/event.h"

ETDevice* ETNewDevice(void) {
    ETDevice *device = (ETDevice*)zmalloc(sizeof(ETDevice));
    memset(device, 0, sizeof(ETDevice));

    device->factory_actor = ETNewFactoryActor();
    pthread_mutex_init(&device->mutex, NULL);

    return device;
}

void ETFreeDevice(ETDevice *device) {
    ETFreeFactoryActor(device->factory_actor);
    pthread_mutex_destroy(&device->mutex);
    zfree(device);
}

pthread_t ETDeviceStartJob(ETDevice *device, const pthread_attr_t *attr,
        void *(*start_routine) (void *), void *arg) {
    pthread_t ntid;
    pthread_create(&ntid, attr, start_routine, arg);
    return ntid;
}
