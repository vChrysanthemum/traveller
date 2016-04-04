#ifndef __UI_UI_H
#define __UI_UI_H

#include <curses.h>
#include <panel.h>

#include "ui/document/document.h"
#include "ui/map.h"

//ColorPair
#define CP_CONSOLE_TAB              1
#define CP_CONSOLE_TAB_ACTIVE       2
#define CP_CONSOLE_TAB_BG           3

#define CONSOLE_MODE_CMD 1

#define UI_MAX_PANELS 32

#define UI_IsWhiteSpace(c) (' ' == c || '\t' == c || '\r' == c || '\n' == c)

typedef struct uiCursor_s {
    int y;
    int x;
    char             utf8char[4];
    int              utf8charPoi;
} uiCursor_t;

typedef struct uiEnv_s {
    int  Ch;
    int  CursorY;
    int  CursorX;
    int  Number;      //已输入的数字
    char SNumber[8];  //已输入的数字
    int  SNumberLen;
} uiEnv_t;

typedef struct uiWindow_s {
    int    Height; //行数 
    int    Width;  //列数 
    int    Startx;
    int    Starty;
    WINDOW *Win;
    PANEL  *Panel;
} uiWindow_t;
uiWindow_t* UI_createWindow(int height, int width, int starty, int startx);
void UI_FreeWindow(uiWindow_t* win);

typedef struct uiPage_s {
    sds          Title;
    sds          Content;
    uiWindow_t   *UIWin;
    uiDocument_t *document;
} uiPage_t;
uiPage_t *UI_NewPage();
void UI_FreePage(uiPage_t *page);
void* UI_LoadPageActor(etActor_t *actor, int args, void **argv);

typedef struct uiConsoleCommand_s {
    sds line;
    sds header;
} uiConsoleCommand_t;

typedef struct uiConsole_s{
    int                mode;
    uiCursor_t         cursor;
    uiConsoleCommand_t cmd;
    uiWindow_t         *TabUIWin;
    uiWindow_t         *UIWin;
} uiConsole_t;

#define COLOR_UNKNOWN -1
int UI_GetColorIntByColorString(char *color);
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
