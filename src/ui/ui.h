#ifndef __UI_UI_H
#define __UI_UI_H

#include <panel.h>
#include <curses.h>
#include "core/errors.h"
#include "core/util.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/zmalloc.h"
#include "ui/map.h"

//ColorPair
#define CP_CONSOLE_TAB              1
#define CP_CONSOLE_TAB_ACTIVE       2
#define CP_CONSOLE_TAB_BG           3

#define CONSOLE_MODE_CMD 1

#define UI_MAX_PANELS 32;

typedef struct {
    int y;
    int x;
    char             utf8char[4];
    int              utf8charPoi;
} UICursor;

typedef struct {
    int  ch;
    int  cursor_y;
    int  cursor_x;
    int  number;      //已输入的数字
    char snumber[8];  //已输入的数字
    int  snumber_len;
} UIEnv;

typedef struct {
    int    height; //行数 
    int    width;  //列数 
    int    startx;
    int    starty;
    WINDOW *win;
    PANEL  *panel;
} UIWindow;

typedef struct {
    int      id;
    sds      title;
    UIWindow *uiwin;
} UIPage;

typedef struct {
    sds line;
    sds header;
} UIConsoleCommand;

typedef struct {
    UIWindow         *uiwin;
    int              mode;
    UICursor         cursor;
    UIConsoleCommand cmd;
} UIConsole;

UIPage* UINewPage();
UIWindow* UIcreateWindow(int height, int width, int starty, int startx);
int UIInit();

void UIinitColor();

typedef void (*UIKeyDownProcessor) (char ch);
int UISubscribeKeyDownEvent(UIKeyDownProcessor subscriber);
int UIUnSubscribeKeyDownEvent(UIKeyDownProcessor subscriber);

void UIinitConsole();
void UIreRenderConsole();

void UIInitMap();

#endif
