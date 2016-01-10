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

#define UI_MAX_PANELS 32;

#define UIMoveUICursorLeft(count) do {\
    ui_env->cursor_x -= count;\
    if (ui_env->cursor_x < 0) {\
        UIMoveCurMapX(ui_env->cursor_x);\
        ui_env->cursor_x = 0;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorRight(count) do {\
    ui_env->cursor_x += count;\
    if (ui_env->cursor_x > ui_rootUIWindow->width) {\
        UIMoveCurMapX(ui_env->cursor_x - ui_rootUIWindow->width);\
        ui_env->cursor_x = ui_rootUIWindow->width;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorUp(count) do {\
    ui_env->cursor_y -= count;\
    if (ui_env->cursor_y < 0) {\
        UIMoveCurMapY(ui_env->cursor_y);\
        ui_env->cursor_y = 0;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorDown(count) do {\
    ui_env->cursor_y += count;\
    if (ui_env->cursor_y > ui_rootUIWindow->height) {\
        UIMoveCurMapY(ui_env->cursor_y - ui_rootUIWindow->height);\
        ui_env->cursor_y = ui_rootUIWindow->height;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

void UIinitColor();

typedef struct {
    int  ch;
    int  cursor_x;
    int  cursor_y;
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

UIWindow* UIcreateWindow(int height, int width, int starty, int startx);

int UIInit();

typedef struct {
    int      id;
    sds      title;
    UIWindow *uiwin;
} UIPage;

typedef struct {
    UIWindow *uiwin;
} UIConsole;

typedef void (*UIKeyDownProcessor) (int ch);

void UIinitConsole();
void UIreRenderConsole();
int UISubscribeKeyDownEvent(UIKeyDownProcessor subscriber);
int UIUnSubscribeKeyDownEvent(UIKeyDownProcessor subscriber);
UIPage* UINewPage();

void UIInitMap();

#endif
