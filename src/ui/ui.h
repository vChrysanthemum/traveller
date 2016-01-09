#ifndef __UI_UI_H
#define __UI_UI_H

#include <panel.h>
#include <curses.h>
#include "core/sds.h"
#include "core/zmalloc.h"
#include "ui/map.h"
#include "ui/color.h"

#define UIMoveUICursorLeft(count) do {\
    ui_env->cursor_x -= count;\
    if (ui_env->cursor_x < 0) {\
        UIMoveCurMapX(ui_env->cursor_x);\
        ui_env->cursor_x = 0;\
    }\
    wmove(g_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorRight(count) do {\
    ui_env->cursor_x += count;\
    if (ui_env->cursor_x > g_rootUIWindow->width) {\
        UIMoveCurMapX(ui_env->cursor_x - g_rootUIWindow->width);\
        ui_env->cursor_x = g_rootUIWindow->width;\
    }\
    wmove(g_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorUp(count) do {\
    ui_env->cursor_y -= count;\
    if (ui_env->cursor_y < 0) {\
        UIMoveCurMapY(ui_env->cursor_y);\
        ui_env->cursor_y = 0;\
    }\
    wmove(g_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorDown(count) do {\
    ui_env->cursor_y += count;\
    if (ui_env->cursor_y > g_rootUIWindow->height) {\
        UIMoveCurMapY(ui_env->cursor_y - g_rootUIWindow->height);\
        ui_env->cursor_y = g_rootUIWindow->height;\
    }\
    wmove(g_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UI_MAX_PANELS 32;

typedef struct {
    int ch;
    int cursor_x;
    int cursor_y;
    int number;          /* 已输入的数字*/
    char snumber[8];     /* 已输入的数字 */
    int snumber_len;
} UIEnv;

typedef struct {
    int height;         /* 行数 */
    int width;          /* 列数 */
    int startx;
    int starty;
    WINDOW *win;
    PANEL *panel;
} UIWindow;

UIWindow* UIcreateWindow(int height, int width, int starty, int startx);

int UIInit();

typedef struct {
    char *name;
} UIconsoleTab;

typedef struct {
    UIWindow     *uiwin;
    UIconsoleTab *main_tab;
} UIConsole;

void UIinitConsole();

#endif
