#include <stdio.h>
#include <panel.h>
#include <string.h>

#include "core/adlist.h"
#include "core/util.h"
#include "core/sds.h"
#include "event/event.h"
#include "ui/ui.h"

#include "g_extern.h"

uiWindow_t *ui_rootuiWindow;
UIMap *ui_curUIMap;

uiEnv_t *ui_env;
list *ui_panels;
uiConsole_t *ui_console;
int ui_width, ui_height; //屏幕宽度、高度
list *ui_pages;
uiPage_t *ui_activePage;

static list *keyDownProcessors;

static void uiLoop() {
    etDevice_t *device = g_mainDevice;

    while(1) {
        halfdelay(2);
        ui_env->ch = getch();
        if (ERR != ui_env->ch) {
            UIKeyDownProcessor proc;
            listNode *node;
            listIter *iter = listGetIterator(keyDownProcessors, AL_START_HEAD);
            while (0 != (node = listNext(iter))) {
                proc = (UIKeyDownProcessor)node->value;
                proc(ui_env->ch);
            }
            listReleaseIterator(iter);
        }

        ET_DeviceFactoryActorLoopOnce(device);
    }

    endwin();
}

int UI_SubscribeKeyDownEvent(UIKeyDownProcessor subscriber) {
    keyDownProcessors = listAddNodeTail(keyDownProcessors, subscriber);
    return ERRNO_OK;
}

int UI_UnSubscribeKeyDownEvent(UIKeyDownProcessor subscriber) {
    listNode *node;
    listIter *iter = listGetIterator(keyDownProcessors, AL_START_HEAD);
    while (0 != (node = listNext(iter))) {
        if (subscriber == node->value) {
            listDelNode(keyDownProcessors, node);
            break;
        }
    }
    listReleaseIterator(iter);
    return ERRNO_OK;
}

int UI_Init() {
    ui_panels = listCreate();
    ui_pages = listCreate();
    keyDownProcessors = listCreate();

    ui_env = (uiEnv_t*)zmalloc(sizeof(uiEnv_t));
    ui_env->number = 1;
    ui_env->snumber[0] = 0;
    ui_env->snumberLen = 0;

    setlocale(LC_ALL, "");  
    initscr();
    clear();
    cbreak();
    noecho();
    //raw();

    getmaxyx(stdscr, ui_height, ui_width);

    UI_PrepareColor();

    UI_initConsole();
    //UI_InitMap();

    top_panel(ui_console->uiwin->panel);
    update_panels();
    doupdate();

    uiPage_t *page;
    page = UI_NewPage("地图");
    mvwprintw(page->uiwin->win, 2, 2, "");
    UI_NewPage("飞船");
    ui_activePage = page;
    UI_reRenderConsole();

    uiLoop();

    return ERRNO_OK;

}
