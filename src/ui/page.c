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
    page->title = sdsempty();
    page->content = sdsempty();
    return page;
}

void UI_FreePage(uiPage_t *page) {
    sdsfree(page->title);
    sdsfree(page->content);
    UI_FreeWindow(page->uiwin);
    zfree(page);
    return;
}

void* UI_LoadPageActor(etActor_t *actor, int args, void **argv) {
    uiPage_t *page = (uiPage_t*)argv[0];

    uiDocument_t *document = UI_NewDocument();
    document->content = page->content;
    UI_ParseHtml(document);

    uiHtmlDom_t *dom = listNodeValue(document->rootDom->children->head);
    TrvLogI("%s", dom->title);
    page->uiwin = UI_createWindow(20, ui_width, 0, 0);
    wprintw(page->uiwin->win, page->content);
    wrefresh(page->uiwin->win);

    return 0;
}
