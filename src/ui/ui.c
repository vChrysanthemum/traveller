#include <stdio.h>
#include <ncurses.h>
#include <panel.h>
#include <string.h>
#include <pthread.h>

#include "core/util.h"
#include "core/zmalloc.h"
#include "core/sds.h"
#include "ui/ui.h"
#include "ui/map.h"

extern UIWin *g_rootUIWin;
extern UICursor *g_cursor;
extern UIMap *g_curUIMap;
extern void *g_tmpPtr;

static UIWin* createUIWin(int height, int width, int starty, int startx) {
    UIWin *win = (UIWin*)zmalloc(sizeof(UIWin));
    win->startx = startx;
    win->starty = starty;
    win->height = height;
    win->width = width;
    win->window = newwin(height, width, starty, startx);
    return win;
}

static void initRootUIWin() {
    int height, width;
    getmaxyx(stdscr, height, width);
    g_rootUIWin = createUIWin(height, width, 0, 0);

    g_cursor = (UICursor*)zmalloc(sizeof(UICursor));
    g_cursor->number = 1;
    g_cursor->snumber[0] = 0;
    g_cursor->snumber_len = 0;

    getmaxyx(stdscr, g_rootUIWin->height, g_rootUIWin->width);
    g_rootUIWin->height-=2; /* 最后一行不可写 */
    g_rootUIWin->width--; /* 最后一列不可写 */
    keypad(g_rootUIWin->window, TRUE);
}

static void* uiLoop(void* _) {

    while(1) {
        g_rootUIWin->ch = wgetch(g_rootUIWin->window);
        if ('0' <= g_rootUIWin->ch && g_rootUIWin->ch <= '9') {
            if (g_cursor->snumber_len > 6) 
                continue;

            g_cursor->snumber[g_cursor->snumber_len] = g_rootUIWin->ch;
            g_cursor->snumber_len++;
            g_cursor->snumber[g_cursor->snumber_len] = 0;

            continue;
        }
        else if(g_cursor->snumber_len > 0) {
            g_cursor->number = atoi(g_cursor->snumber);
            g_cursor->snumber_len = 0;
            g_cursor->snumber[0] = 0;
        }


        if (KEY_UP == g_rootUIWin->ch || 'k' == g_rootUIWin->ch) {
            UIMoveUICursorUp(g_cursor->number);
        }

        else if (KEY_DOWN == g_rootUIWin->ch || 'j' == g_rootUIWin->ch) {
            UIMoveUICursorDown(g_cursor->number);
        }

        else if (KEY_LEFT == g_rootUIWin->ch || 'h' == g_rootUIWin->ch) {
            UIMoveUICursorLeft(g_cursor->number);
        }

        else if (KEY_RIGHT == g_rootUIWin->ch || 'l' == g_rootUIWin->ch) {
            UIMoveUICursorRight(g_cursor->number);
        }

        g_cursor->number = 1;

        if (KEY_F(1) == g_rootUIWin->ch) break; /* ESC */ 

        wrefresh(g_rootUIWin->window);
    }

    return 0;
}

void UIInit() {
    sds mapJSON;
    char dir[ALLOW_PATH_SIZE] = {""};

    initscr();                   /* start the curses mode */
    clear();
    cbreak();
    noecho();
    //raw();

    initRootUIWin();

    /* 画首幅地图 */
    //sprintf(dir, "%s/arctic.map.json", m_galaxiesdir);
    sprintf(dir, "/Users/j/github/my/traveller/galaxies/gemini/client/arctic.map.json");
    mapJSON = fileGetContent(dir);
    g_curUIMap = UIParseMap(mapJSON);

    UIDrawMap();
    wrefresh(g_rootUIWin->window);


    /* 光标置中 */
    g_cursor->x = g_rootUIWin->width / 2;
    g_cursor->y = g_rootUIWin->height / 2;
    wmove(g_rootUIWin->window, g_cursor->y, g_cursor->x);

    /* 循环 */
    pthread_t ntid;
    pthread_create(&ntid, NULL, uiLoop, NULL);

    endwin();
}
