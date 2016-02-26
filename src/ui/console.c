#include <string.h>
#include "ui/ui.h"
#include "g_extern.h"
#include "ui/extern.h"

static uiWindow_t *uiwin;
static WINDOW   *win;
static uiWindow_t *tabuiwin;
static WINDOW   *tabwin;
static uiCursor_t *cursor;
static uiConsoleCommand_t *cmd;

static void renderTabs() {
    wattron(tabwin, COLOR_PAIR(CP_CONSOLE_TAB_BG));
    for (int i = 0; i < ui_width; i++) {
        mvwaddch(tabwin, 0, i, ' ');
    }
    wattroff(tabwin, COLOR_PAIR(CP_CONSOLE_TAB_BG));

    wmove(tabwin, 0, 0);

    listNode *ln;
    uiPage_t *page;
    listIter *li = listGetIterator(ui_pages, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        page = (uiPage_t*)ln->value;

        if (page == ui_activePage) {
            wattron(tabwin, COLOR_PAIR(CP_CONSOLE_TAB_ACTIVE));
        } else {
            wattron(tabwin, COLOR_PAIR(CP_CONSOLE_TAB));
        }

        wprintw(tabwin, " ");
        wprintw(tabwin, page->title);
        wprintw(tabwin, " ");

        if (page == ui_activePage) {
            wattroff(tabwin, COLOR_PAIR(CP_CONSOLE_TAB_ACTIVE));
        } else {
            wattroff(tabwin, COLOR_PAIR(CP_CONSOLE_TAB));
        }
    }
    listReleaseIterator(li);

    wmove(win, cursor->y, cursor->x);
}

static void cmdAddch(int ch) {
    mvwaddch(win, cursor->y, cursor->x, ch);

    if ('\n' == ch || cursor->x >= ui_width-1) {
        cursor->y++;
        cursor->x = 0;
    } else {
        cursor->x++;
    }
}

static void cmdPrintw(char *str) {
    mvwprintw(win, cursor->y, cursor->x, str);

    int str_width = utf8StrWidth(str);
    if (cursor->x + str_width >= ui_width) {
        cursor->y++;
        cursor->x = cursor->x + str_width - ui_width;

    } else {
        cursor->x += str_width;
    }

    wmove(win, cursor->y, cursor->x);
}

static void keyDownProcessorCmdMode (int ch) {
    cmd->line = sdscatlen(cmd->line, &ch, 1);
    if (ch < 0) {
        cursor->utf8charPoi++;
        if (cursor->utf8charPoi >= 2) {
            cursor->utf8char[cursor->utf8charPoi] = ch;
            cmdPrintw(cursor->utf8char);
            cursor->utf8charPoi = -1;

        } else {
            cursor->utf8char[cursor->utf8charPoi] = ch;
        }

    } else {
        cmdAddch(ch);
    }
}

static void keyDownProcessor (int ch) {
    switch(ui_console->mode) {
        case CONSOLE_MODE_CMD:
            keyDownProcessorCmdMode(ch);
            break;
    }

    wrefresh(win);
}

static void prepareCmdMode() {
    cmdPrintw(cmd->header);
    cmdAddch(' ');
    ui_console->mode = CONSOLE_MODE_CMD;
}

void UI_initConsole() {
    ui_console = (uiConsole_t*)zmalloc(sizeof(uiConsole_t));

    ui_console->tabuiwin = UI_createWindow(1, ui_width, ui_height-6, 0);
    ui_console->uiwin = UI_createWindow(5, ui_width, ui_height-5, 0);
    ui_console->cursor.y = 0;
    ui_console->cursor.x = 0;
    ui_console->cmd.line = sdsempty();
    ui_console->cmd.header = sdsnew("大乔木号$");

    memset(ui_console->cursor.utf8char, 0, 4*sizeof(char));
    ui_console->cursor.utf8charPoi = -1;

    uiwin = ui_console->uiwin;
    win = uiwin->win;
    tabuiwin = ui_console->tabuiwin;
    tabwin = tabuiwin->win;
    cursor = &ui_console->cursor;
    cmd = &ui_console->cmd;

    prepareCmdMode();
    UI_SubscribeKeyDownEvent((UIKeyDownProcessor)keyDownProcessor);

    wrefresh(tabwin);
    wrefresh(win);
}

void UI_reRenderConsole() {
    renderTabs();

    wrefresh(tabwin);
    wrefresh(win);
}
