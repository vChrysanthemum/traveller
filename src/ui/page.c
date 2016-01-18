#include "ui.h"
#include "g_extern.h"
#include "ui/extern.h"

UIPage* UINewPage(char *title) {
    UIPage *page = (UIPage*)zmalloc(sizeof(UIPage));
    page->id = ui_pages->len + 1;
    page->title = sdsnew(title);

    page->uiwin = UIcreateWindow(
            ui_height - ui_console->uiwin->height,
            ui_width, 0, 0);

    ui_pages = listAddNodeTail(ui_pages, page);

    return page;
}
