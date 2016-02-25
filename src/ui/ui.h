#ifndef __UI_UI_H
#define __UI_UI_H

#include <curses.h>
#include <panel.h>
#include "core/errors.h"
#include "core/util.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/zmalloc.h"
#include "ui/map.h"
#include "ui/document/document.h"

//ColorPair
#define CP_CONSOLE_TAB              1
#define CP_CONSOLE_TAB_ACTIVE       2
#define CP_CONSOLE_TAB_BG           3

#define CONSOLE_MODE_CMD 1

#define UI_MAX_PANELS 32

typedef struct uiCursor_s {
    int y;
    int x;
    char             utf8char[4];
    int              utf8charPoi;
} uiCursor_t;

typedef struct uiEnv_s {
    int  ch;
    int  cursorY;
    int  cursorX;
    int  number;      //已输入的数字
    char snumber[8];  //已输入的数字
    int  snumberLen;
} uiEnv_t;

typedef struct uiWindow_s {
    int    height; //行数 
    int    width;  //列数 
    int    startx;
    int    starty;
    WINDOW *win;
    PANEL  *panel;
} uiWindow_t;
uiWindow_t* UI_createWindow(int height, int width, int starty, int startx);
void UI_FreeWindow(uiWindow_t* win);

typedef struct uiPage_s {
    sds      title;
    sds      content;
    uiWindow_t *uiwin;
} uiPage_t;
uiPage_t *UI_NewPage();
void UI_FreePage(uiPage_t *page);
void* UI_LoadPageActor(etActor_t *actor, int args, void **argv);

typedef struct uiConsoleCommand_s {
    sds line;
    sds header;
} uiConsoleCommand_t;

typedef struct uiConsole_s{
    uiWindow_t         *tabuiwin;
    uiWindow_t         *uiwin;
    int              mode;
    uiCursor_t         cursor;
    uiConsoleCommand_t cmd;
} uiConsole_t;

int UI_PrepareColor();
int UI_Prepare();
int UI_Init();

typedef void (*UIKeyDownProcessor) (char ch);
int UI_SubscribeKeyDownEvent(UIKeyDownProcessor subscriber);
int UI_UnSubscribeKeyDownEvent(UIKeyDownProcessor subscriber);

void UI_initConsole();
void UI_reRenderConsole();

void UI_InitMap();

#endif
