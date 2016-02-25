#include <stdio.h>
#include <string.h>

#include "core/adlist.h"
#include "core/util.h"
#include "core/sds.h"
#include "event/event.h"
#include "ui/ui.h"

#include "g_extern.h"

uiWindow_t  *ui_rootuiWindow;
UIMap       *ui_curUIMap;

uiEnv_t     *ui_env;
list        *ui_panels;
uiConsole_t *ui_console;
int         ui_width, ui_height; //屏幕宽度、高度
list        *ui_pages;
uiPage_t    *ui_activePage;

etDevice_t  *ui_device;

int ui_ColorPair[8][8];

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
            while (NULL != (node = listNext(iter))) {
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
    while (NULL != (node = listNext(iter))) {
        if (subscriber == node->value) {
            listDelNode(keyDownProcessors, node);
            break;
        }
    }
    listReleaseIterator(iter);
    return ERRNO_OK;
}

static void UI_PrepareLoadPageActor() {
    etFactoryActor_t *factoryActor = ui_device->factoryActor;
    etChannelActor_t *channelActor = ET_NewChannelActor();
    channelActor->key = stringnew("/loadpage");
    ET_FactoryActorAppendChannel(factoryActor, channelActor);

    etActor_t *actor = ET_FactoryActorNewActor(factoryActor);
    actor->proc = UI_LoadPageActor;
    ET_SubscribeChannel(actor, channelActor);
}

int UI_Prepare() {
    UI_PrepareDocument();

    ui_panels = listCreate();
    ui_pages = listCreate();
    keyDownProcessors = listCreate();

    ui_env = (uiEnv_t*)zmalloc(sizeof(uiEnv_t));
    memset(ui_env, 0, sizeof(uiEnv_t));

    setlocale(LC_ALL, "");

    ui_device = g_mainDevice;
    UI_PrepareLoadPageActor();

    return ERRNO_OK;
}

int UI_Init() {
    initscr();
    clear();
    noecho();
    //cbreak();
    raw();

    getmaxyx(stdscr, ui_height, ui_width);

    UI_PrepareColor();

    UI_initConsole();
    //UI_InitMap();

    top_panel(ui_console->uiwin->panel);
    update_panels();
    doupdate();

    uiLoop();

    return ERRNO_OK;

}
