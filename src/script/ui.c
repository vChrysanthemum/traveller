#include "script/script.h"
#include "script/extern.h"
#include "ui/ui.h"
#include "ui/extern.h"

int STUILoadPage(lua_State *L) {
    UIPage *page = UINewPage();
    page->content = sdscat(page->content, lua_tostring(L, -1));

    ETActorEvent *event =  ETFactoryActorNewEvent(st_device->factoryActor);
    event->channel = sdsnew("/loadpage");
    event->mailArgs = 1;
    event->mailArgv = zmalloc(event->mailArgs * sizeof(void*));
    event->mailArgv[0] = page;

    ETDeviceAppendEvent(ui_device, event);
    return 0;
}
