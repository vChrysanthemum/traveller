#include <stdio.h>
#include <panel.h>
#include <string.h>

#include "core/adlist.h"
#include "core/util.h"
#include "core/sds.h"
#include "ui/ui.h"

UIWindow *ui_rootUIWindow;
UIMap *ui_curUIMap;

UIEnv *ui_env;
list *ui_panels;
UIConsole *ui_console;
int ui_width, ui_height; //屏幕宽度、高度
list *ui_pages;
UIPage *ui_activePage;

static list *keyDownProcessors;

static void uiLoop() {
    while(1) {
        ui_env->ch = getch();

        UIKeyDownProcessor proc;
        listNode *node;
        listIter *iter = listGetIterator(keyDownProcessors, AL_START_HEAD);
        while (NULL != (node = listNext(iter))) {
            proc = (UIKeyDownProcessor)node->value;
            proc(ui_env->ch);
        }
        listReleaseIterator(iter);
    }

    endwin();
}

int UISubscribeKeyDownEvent(UIKeyDownProcessor subscriber) {
    keyDownProcessors = listAddNodeTail(keyDownProcessors, subscriber);
    return ERRNO_OK;
}

int UIUnSubscribeKeyDownEvent(UIKeyDownProcessor subscriber) {
    listNode *node;
    listIter *iter = listGetIterator(keyDownProcessors, AL_START_HEAD);
    while (NULL != (node = listNext(iter))) {
        if (subscriber == node->value) {
            listDelNode(keyDownProcessors, node);
            break;
        }
    }
    listReleaseIterator(iter);
    return ERRNO_OK;
}

int UIInit() {
    ui_panels = listCreate();
    ui_pages = listCreate();
    keyDownProcessors = listCreate();

    ui_env = (UIEnv*)zmalloc(sizeof(UIEnv));
    ui_env->number = 1;
    ui_env->snumber[0] = 0;
    ui_env->snumber_len = 0;

    setlocale(LC_ALL, "");  
    initscr();
    clear();
    cbreak();
    noecho();
    //raw();

    getmaxyx(stdscr, ui_height, ui_width);

    UIinitColor();

    UIinitConsole();
    //UIInitMap();

    top_panel(ui_console->uiwin->panel);
    update_panels();
    doupdate();

    UIPage *page;
    page = UINewPage("地图");
    mvwprintw(page->uiwin->win, 2, 2, "");
    UINewPage("飞船");
    ui_activePage = page;
    UIreRenderConsole();

    uiLoop();

    return ERRNO_OK;

}
