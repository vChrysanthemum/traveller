#ifndef __UI_UI_H
#define __UI_UI_H

#include <ncurses.h>

#define moveCursorLeft(count) do {\
    g_cursor->x -= count;\
    if (g_cursor->x < 0) {\
        moveCurMapX(g_cursor->x);\
        g_cursor->x = 0;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

#define moveCursorRight(count) do {\
    g_cursor->x += count;\
    if (g_cursor->x > g_rootWin->width) {\
        moveCurMapX(g_cursor->x - g_rootWin->width);\
        g_cursor->x = g_rootWin->width;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

#define moveCursorUp(count) do {\
    g_cursor->y -= count;\
    if (g_cursor->y < 0) {\
        moveCurMapY(g_cursor->y);\
        g_cursor->y = 0;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

#define moveCursorDown(count) do {\
    g_cursor->y += count;\
    if (g_cursor->y > g_rootWin->height) {\
        moveCurMapY(g_cursor->y - g_rootWin->height);\
        g_cursor->y = g_rootWin->height;\
    }\
    move(g_cursor->y, g_cursor->x);\
} while(0);

typedef struct {
    int x;
    int y;
    int number;
    char snumber[8];     /* 已输入的数字 */
    int snumber_len;
} Cursor;

enum WinMode mode = {};

typedef struct {
    int height;         /* 行数 */
    int width;            /* 列数 */
    int startx;
    int starty;
    int ch;
    enum WinMode mode;
    WINDOW *window;
} Win;

void initUI();
void winAddCh(Win *win, int y, int x, char ch);

#endif
