#include "lua.h"

#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/extern.h"

#include "event/event.h"
#include "ui/ui.h"

#include "g_extern.h"
#include "ui/extern.h"

uiPage_t *UI_NewPage() {
    uiPage_t *page = (uiPage_t*)zmalloc(sizeof(uiPage_t));
    memset(page, 0, sizeof(uiPage_t));
    page->Title = sdsempty();
    page->Content = sdsempty();
    return page;
}

void UI_FreePage(uiPage_t *page) {
    sdsfree(page->Title);
    sdsfree(page->Content);
    UI_FreeWindow(page->UIWin);
    zfree(page);
    return;
}

void* UI_LoadPageActor(etActor_t *actor, int args, void **argv) {
    uiPage_t *page = (uiPage_t*)argv[0];

    page->UIWin = UI_createWindow(20, ui_width, 0, 0);
    page->document = UI_ParseDocument(page->Content);

    wprintw(page->UIWin->Win, page->Content);
    wrefresh(page->UIWin->Win);

    return 0;
}
