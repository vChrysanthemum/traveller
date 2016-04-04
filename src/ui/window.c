#include "core/zmalloc.h"
#include "core/adlist.h"
#include "core/sds.h"
#include "core/dict.h"

#include "event/event.h"
#include "ui/ui.h"

extern list *ui_panels;

uiWindow_t* UI_createWindow(int height, int width, int starty, int startx) {
    uiWindow_t *win = (uiWindow_t*)zmalloc(sizeof(uiWindow_t));
    win->Startx = startx;
    win->Starty = starty;
    win->Height = height;
    win->Width = width;
    win->Win = newwin(height, width, starty, startx);
    win->Panel = new_panel(win->Win);
    ui_panels = listAddNodeTail(ui_panels, win->Panel);
    return win;
}

void UI_FreeWindow(uiWindow_t *win) {
    del_panel(win->Panel);
    delwin(win->Win);
    zfree(win);
}
