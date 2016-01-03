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
    listAddNodeTail(ui_panels, win->panel);
    return win;
}
