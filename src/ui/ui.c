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

ETDevice *ui_device;

static list *keyDownProcessors;

static void uiLoop() {
    ETDevice *device = g_mainDevice;

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

        ETDeviceFactoryActorLoopOnce(device);
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

static void UIPrepareLoadPageActor() {
    ETFactoryActor *factoryActor = ui_device->factoryActor;
    ETChannelActor *channelActor = ETNewChannelActor();
    channelActor->key = stringnew("/loadpage");
    ETFactoryActorAppendChannel(factoryActor, channelActor);

    ETActor *actor = ETFactoryActorNewActor(factoryActor);
    actor->proc = UILoadPageActor;
    ETSubscribeChannel(actor, channelActor);
}

int UIPrepare() {
    UIHtmlPrepare();

    ui_panels = listCreate();
    ui_pages = listCreate();
    keyDownProcessors = listCreate();

    ui_env = (UIEnv*)zmalloc(sizeof(UIEnv));
    memset(ui_env, 0, sizeof(UIEnv));

    setlocale(LC_ALL, "");

    ui_device = g_mainDevice;
    UIPrepareLoadPageActor();

    return ERRNO_OK;
}

int UIInit() {
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

    uiLoop();

    return ERRNO_OK;

}
