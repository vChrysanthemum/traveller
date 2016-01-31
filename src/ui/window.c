#include "core/zmalloc.h"
#include "core/adlist.h"
#include "ui/ui.h"

extern list *ui_panels;

UIWindow* UIcreateWindow(int height, int width, int starty, int startx) {
    UIWindow *win = (UIWindow*)zmalloc(sizeof(UIWindow));
    win->startx = startx;
    win->starty = starty;
    win->height = height;
    win->width = width;
    win->win = newwin(height, width, starty, startx);
    win->panel = new_panel(win->win);
    ui_panels = listAddNodeTail(ui_panels, win->panel);
    return win;
}

void UIFreeWindow(UIWindow *win) {
    del_panel(win->panel);
    delwin(win->win);
    zfree(win);
}
