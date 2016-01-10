#include "ui/ui.h"
#include "extern.h"

static void renderTabs() {
    WINDOW *win = ui_console->uiwin->win;

    wattron(win, COLOR_PAIR(CP_CONSOLE_TAB_BG));
    for (int i = 0; i < ui_width; i++) {
        mvwaddch(win, 0, i, ' ');
    }
    wattroff(win, COLOR_PAIR(CP_CONSOLE_TAB_BG));

    TrvLogD("hi");
    wmove(win, 0, 0);

    listNode *node;
    UIPage *page;
    listIter *iter = listGetIterator(ui_pages, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {
        page = (UIPage*)node->value;

        if (page == ui_activePage) {
            wattron(win, COLOR_PAIR(CP_CONSOLE_TAB_ACTIVE));
        } else {
            wattron(win, COLOR_PAIR(CP_CONSOLE_TAB));
        }

        wprintw(win, " ");
        wprintw(win, page->title);
        wprintw(win, " ");

        if (page == ui_activePage) {
            wattroff(win, COLOR_PAIR(CP_CONSOLE_TAB_ACTIVE));
        } else {
            wattroff(win, COLOR_PAIR(CP_CONSOLE_TAB));
        }
    }
    listReleaseIterator(iter);
}

static void keyDownProcessor (int ch) {
    wrefresh(ui_console->uiwin->win);
}

void UIinitConsole() {
    ui_console = (UIConsole*)zmalloc(sizeof(UIConsole));

    ui_console->uiwin = UIcreateWindow(6, ui_width, ui_height-6, 0);

    UISubscribeKeyDownEvent((UIKeyDownProcessor)keyDownProcessor);

    UIPage *page;
    page = UINewPage("雷达");
    mvwprintw(page->uiwin->win, 2, 2, "我是雷达");
    UINewPage("飞船");
    ui_activePage = page;
    UIreRenderConsole();

    wrefresh(ui_console->uiwin->win);
}

void UIreRenderConsole() {
    WINDOW *win = ui_console->uiwin->win;

    renderTabs();

    mvwprintw(win, 1, 0, ":Ground control to major Tom.");
}
