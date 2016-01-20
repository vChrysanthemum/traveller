#include <stdio.h>
#include <panel.h>
#include <string.h>

#include "core/adlist.h"
#include "core/util.h"
#include "core/sds.h"
#include "event/event.h"
#include "ui/ui.h"

#include "g_extern.h"

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
    ETDevice *device = g_mainDevice;
    ETFactoryActor *factoryActor = device->factory_actor;
    list *_l;
    listIter *iter;
    listNode *node;

    while(1) {
        halfdelay(2);
        ui_env->ch = getch();
        if (ERR != ui_env->ch) {
            UIKeyDownProcessor proc;
            listNode *node;
            listIter *iter = listGetIterator(keyDownProcessors, AL_START_HEAD);
            while (NULL != (node = listNext(iter))) {
                proc = (UIKeyDownProcessor)node->value;
                proc(ui_env->ch);
            }
            listReleaseIterator(iter);
        }

        if (0 == factoryActor->running_event_list->len) {
            _l = factoryActor->running_event_list;
            factoryActor->running_event_list = factoryActor->waiting_event_list;
            factoryActor->waiting_event_list = _l;
        }

        iter = listGetIterator(factoryActor->running_event_list, AL_START_HEAD);
        while (NULL != (node = listNext(iter))) {
            ETFactoryActorProcessEvent(factoryActor, (ETActorEvent*)node->value);

            listDelNode(factoryActor->running_event_list, node);
        }

        _l = ETDevicePopEventList(device);
        if (0 != _l) {
            iter = listGetIterator(_l, AL_START_HEAD);
            while (NULL != (node = listNext(iter))) {
                ETFactoryActorProcessEvent(factoryActor, (ETActorEvent*)node->value);

                listDelNode(_l, node);
            }
            listRelease(_l);
        }
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
