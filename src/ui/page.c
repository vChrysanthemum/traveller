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
    UIHtmlDom *dom = listNodeValue(UIParseHtml(page->content)->children->head);
    TrvLogI("%s", dom->title);
    page->uiwin = UIcreateWindow(20, ui_width, 0, 0);
    wprintw(page->uiwin->win, page->content);
    wrefresh(page->uiwin->win);

    return 0;
}
