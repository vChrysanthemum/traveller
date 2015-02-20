#include <ncurses.h>
#include <panel.h>
#include <string.h>

#include "core/util.h"
#include "core/zmalloc.h"
#include "core/sds.h"
#include "ui/ui.h"
#include "ui/map.h"

extern Win *g_rootWin;
extern Cursor *g_cursor;
extern char g_planetdir[ALLOW_PATH_SIZE];

static void initRootWin() {
    g_rootWin = (Win*)zmalloc(sizeof(Win));
    g_rootWin->window = stdscr;
    g_rootWin->startx = 0;
    g_rootWin->starty = 0;

    g_cursor = (Cursor*)zmalloc(sizeof(Cursor));
    g_cursor->number = 1;
    g_cursor->snumber[0] = 0x00;
    g_cursor->snumber_len = 0;

    getmaxyx(stdscr, g_rootWin->height, g_rootWin->width);
    g_rootWin->height--; /* 最后一列不可写 */
    g_rootWin->width--; /* 最后一行不可写 */
}

static Win* createWin(int height, int width, int starty, int startx) {
    Win *win = (Win*)zmalloc(sizeof(Win));
    win->startx = startx;
    win->starty = starty;
    win->height = height;
    win->width = width;
    win->window = newwin(height, width, starty, startx);
    wrefresh(win->window);
    return win;
}

void winAddCh(Win *win, int y, int x, char ch) {
    mvwaddch(win->window, y, x, ch);
}

void initUI() {
    sds mapJSON;
    Map *map;
    char dir[ALLOW_PATH_SIZE] = {""};

    initscr();                   /* start the curses mode */
    //raw();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    initRootWin();

    refresh();
    Win *win = createWin(
            (int)(g_rootWin->height * 0.9),
            (int)(g_rootWin->width * 0.9),
            (int)(g_rootWin->height * 0.05),
            (int)(g_rootWin->width * 0.05)
            );
    //box(win->window, 0, 0);

    sprintf(dir, "%s/arctic.map.json", g_planetdir);
    mapJSON = fileGetContent(dir);
    map = parseMap(mapJSON);

    drawMap(map);
    wrefresh(win->window);


    g_cursor->x = g_rootWin->width / 2;
    g_cursor->y = g_rootWin->height / 2;
    move(g_cursor->y, g_cursor->x);


    while(1) {
        g_rootWin->ch = getch();
        mvwin(stdscr, g_cursor->y, g_cursor->x);
        if ('0' <= g_rootWin->ch && g_rootWin->ch <= '9') {
            if (g_cursor->snumber_len > 6) 
                continue;

            g_cursor->snumber[g_cursor->snumber_len] = g_rootWin->ch;
            g_cursor->snumber_len++;
            g_cursor->snumber[g_cursor->snumber_len] = 0x00;

            continue;
        }
        else if(g_cursor->snumber_len > 0) {
            g_cursor->number = atoi(g_cursor->snumber);
            g_cursor->snumber_len = 0;
            g_cursor->snumber[0] = 0x00;
        }


        if (KEY_UP == g_rootWin->ch || 'k' == g_rootWin->ch) {
            moveCursorUp(g_cursor->number);
        }

        else if (KEY_DOWN == g_rootWin->ch || 'j' == g_rootWin->ch) {
            moveCursorDown(g_cursor->number);
        }

        else if (KEY_LEFT == g_rootWin->ch || 'h' == g_rootWin->ch) {
            moveCursorLeft(g_cursor->number);
        }

        else if (KEY_RIGHT == g_rootWin->ch || 'l' == g_rootWin->ch) {
            moveCursorRight(g_cursor->number);
        }

        g_cursor->number = 1;

        if (KEY_F(1) == g_rootWin->ch) break; /* ESC */ 
        refresh();
    }


    getch();
    endwin();
}
