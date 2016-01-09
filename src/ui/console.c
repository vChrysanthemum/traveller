#include "ui/ui.h"

extern UIConsole *ui_console;
extern int ui_width, ui_height;

void UIinitConsole() {
    ui_console = (UIConsole*)zmalloc(sizeof(UIConsole));
    ui_console->uiwin = UIcreateWindow(4, ui_width, ui_height-4, 0);

    WINDOW *win = ui_console->uiwin->win;

    wattron(win, COLOR_PAIR(CP_CONSOLE_TAB_BG));
    for (int i = 0; i < ui_width; i++) {
        mvwaddch(win, 0, i, ' ');
    }
    wattroff(win, COLOR_PAIR(CP_CONSOLE_TAB_BG));

    sds tab_spaceship = sdsnew(" 飞船 ");

    wattron(win, COLOR_PAIR(CP_CONSOLE_TAB_ACTIVE));
    mvwprintw(win, 0, 1, tab_spaceship);
    wattroff(win, COLOR_PAIR(CP_CONSOLE_TAB_ACTIVE));

    sds tab_map = sdsnew(" 地图 ");

    wattron(win, COLOR_PAIR(CP_CONSOLE_TAB));
    wprintw(win, tab_map);
    wattroff(win, COLOR_PAIR(CP_CONSOLE_TAB));

    mvwprintw(win, 1, 0, ":Ground control to major Tom.");
}
