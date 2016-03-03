#include "core/zmalloc.h"
#include "core/adlist.h"
#include "core/sds.h"
#include "core/dict.h"

#include "event/event.h"
#include "ui/ui.h"

extern list *ui_panels;

uiWindow_t* UI_createWindow(int height, int width, int starty, int startx) {
    uiWindow_t *win = (uiWindow_t*)zmalloc(sizeof(uiWindow_t));
    win->startx = startx;
    win->starty = starty;
    win->height = height;
    win->width = width;
    win->win = newwin(height, width, starty, startx);
    win->panel = new_panel(win->win);
    ui_panels = listAddNodeTail(ui_panels, win->panel);
    return win;
}

void UI_FreeWindow(uiWindow_t *win) {
    del_panel(win->panel);
    delwin(win->win);
    zfree(win);
}
