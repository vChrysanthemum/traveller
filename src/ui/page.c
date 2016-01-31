#include "ui.h"
#include "g_extern.h"
#include "ui/extern.h"

UIPage *UINewPage() {
    UIPage *page = (UIPage*)zmalloc(sizeof(UIPage));
    memset(page, 0, sizeof(UIPage));
    page->title = sdsempty();
    page->content = sdsempty();
    return page;
}

void UIFreePage(UIPage *page) {
    sdsfree(page->title);
    sdsfree(page->content);
    UIFreeWindow(page->uiwin);
    zfree(page);
    return;
}

void* UILoadPageActor(ETActor *actor, int args, void **argv) {
    UIPage *page = (UIPage*)argv[0];
    TrvLogI("%s", page->content);

    return 0;
}
