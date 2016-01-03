#include <stdio.h>
#include <ncurses.h>
#include <panel.h>
#include <string.h>
#include <pthread.h>

#include "core/adlist.h"
#include "core/util.h"
#include "core/sds.h"
#include "ui/ui.h"

extern UIWindow *g_rootUIWindow;
extern UIMap *g_curUIMap;

UIEnv *ui_env;
list *ui_panels;
UIConsole *ui_console;
int ui_width, ui_height; //屏幕宽度、高度

static void initRootUIWindow() {
    g_rootUIWindow = UIcreateWindow(ui_height-5, ui_width, 0, 0);

    ui_env = (UIEnv*)zmalloc(sizeof(UIEnv));
    ui_env->number = 1;
    ui_env->snumber[0] = 0;
    ui_env->snumber_len = 0;

    getmaxyx(stdscr, g_rootUIWindow->height, g_rootUIWindow->width);
    //g_rootUIWindow->height--; /* 最后一行不可写 */
    //g_rootUIWindow->width--; /* 最后一列不可写 */
    keypad(g_rootUIWindow->win, TRUE);
}

static void* uiLoop(void* _) {

    while(1) {
        ui_env->ch = getch();
        if ('0' <= ui_env->ch && ui_env->ch <= '9') {
            if (ui_env->snumber_len > 6) {
                continue;
            }

            ui_env->snumber[ui_env->snumber_len] = ui_env->ch;
            ui_env->snumber_len++;
            ui_env->snumber[ui_env->snumber_len] = 0;

            continue;
        }
        else if(ui_env->snumber_len > 0) {
            ui_env->number = atoi(ui_env->snumber);
            ui_env->snumber_len = 0;
            ui_env->snumber[0] = 0;
        }


        if (KEY_UP == ui_env->ch || 'k' == ui_env->ch) {
            UIMoveUICursorUp(ui_env->number);
        }

        else if (KEY_DOWN == ui_env->ch || 'j' == ui_env->ch) {
            UIMoveUICursorDown(ui_env->number);
        }

        else if (KEY_LEFT == ui_env->ch || 'h' == ui_env->ch) {
            UIMoveUICursorLeft(ui_env->number);
        }

        else if (KEY_RIGHT == ui_env->ch || 'l' == ui_env->ch) {
            UIMoveUICursorRight(ui_env->number);
        }

        ui_env->number = 1;

        if (KEY_F(1) == ui_env->ch) break; /* ESC */ 

        refresh();
        wrefresh(g_rootUIWindow->win);
        wrefresh(ui_console->uiwin->win);
    }

    endwin();

    return 0;
}

int UIInit() {
    sds mapJSON;
    char dir[ALLOW_PATH_SIZE] = {""};

    ui_panels = listCreate();

    initscr();
    clear();
    cbreak();
    noecho();
    //raw();

    getmaxyx(stdscr, ui_height, ui_width);

    UIinitColor();

    initRootUIWindow();

    UIinitConsole();

    /* 画首幅地图 */
    //sprintf(dir, "%s/arctic.map.json", m_galaxiesdir);
    sprintf(dir, "/Users/j/github/my/traveller/galaxies/gemini/client/arctic.map.json");
    mapJSON = fileGetContent(dir);
    g_curUIMap = UIParseMap(mapJSON);

    UIDrawMap();

    /* 光标置中 */
    ui_env->cursor_x = g_rootUIWindow->width / 2;
    ui_env->cursor_y = g_rootUIWindow->height / 2;
    wmove(g_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);

    refresh();
    wrefresh(g_rootUIWindow->win);
    wrefresh(ui_console->uiwin->win);

    /* 循环 */
    pthread_t ntid;
    pthread_create(&ntid, NULL, uiLoop, NULL);

    return ERRNO_OK;
}
