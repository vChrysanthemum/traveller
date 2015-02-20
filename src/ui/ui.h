#ifndef __UI_UI_H
#define __UI_UI_H

#include <ncurses.h>

#define UIMoveUICursorLeft(count) do {\
    g_cursor->x -= count;\
    if (g_cursor->x < 0) {\
        UIMoveCurUIMapX(g_cursor->x);\
        g_cursor->x = 0;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

#define UIMoveUICursorRight(count) do {\
    g_cursor->x += count;\
    if (g_cursor->x > g_rootUIWin->width) {\
        UIMoveCurUIMapX(g_cursor->x - g_rootUIWin->width);\
        g_cursor->x = g_rootUIWin->width;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

#define UIMoveUICursorUp(count) do {\
    g_cursor->y -= count;\
    if (g_cursor->y < 0) {\
        UIMoveCurUIMapY(g_cursor->y);\
        g_cursor->y = 0;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

#define UIMoveUICursorDown(count) do {\
    g_cursor->y += count;\
    if (g_cursor->y > g_rootUIWin->height) {\
        UIMoveCurUIMapY(g_cursor->y - g_rootUIWin->height);\
        g_cursor->y = g_rootUIWin->height;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

typedef struct {
    int x;
    int y;
    int number;
    char snumber[8];     /* 已输入的数字 */
    int snumber_len;
} UICursor;

//enum UIWinMode mode = {};

typedef struct {
    int height;         /* 行数 */
    int width;            /* 列数 */
    int startx;
    int starty;
    int ch;
    //enum UIWinMode mode;
    WINDOW *window;
} UIWin;

void UIinit();

#endif
