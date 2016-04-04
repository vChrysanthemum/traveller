#include <string.h>

#include "lua.h"

#include "core/sds.h"
#include "core/dict.h"
#include "core/adlist.h"
#include "core/util.h"
#include "core/zmalloc.h"
#include "core/frozen.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/trash/map.h"
#include "g_extern.h"
#include "ui/extern.h"

extern uiWindow_t *ui_rootuiWindow;
extern UIMap *ui_curUIMap;

static void *g_tmpPtr;

static void keyDownProcessor (int ch) {
    if ('0' <= ui_env->Ch && ui_env->Ch <= '9') {
        if (ui_env->SNumberLen > 6) {
            return;
        }

        ui_env->SNumber[ui_env->SNumberLen] = ui_env->Ch;
        ui_env->SNumberLen++;
        ui_env->SNumber[ui_env->SNumberLen] = 0;

        return;
    } else if (ui_env->SNumberLen > 0) {
        ui_env->Number = atoi(ui_env->SNumber);
        ui_env->SNumberLen = 0;
        ui_env->SNumber[0] = 0;
    }


    if (KEY_UP == ui_env->Ch || 'k' == ui_env->Ch) {
        UI_MoveuiCursorUp(ui_env->Number);

    } else if (KEY_DOWN == ui_env->Ch || 'j' == ui_env->Ch) {
        UIMoveuiCursor_tDown(ui_env->Number);

    } else if (KEY_LEFT == ui_env->Ch || 'h' == ui_env->Ch) {
        UIMoveuiCursor_tLeft(ui_env->Number);

    } else if (KEY_RIGHT == ui_env->Ch || 'l' == ui_env->Ch) {
        UIMoveuiCursor_tRight(ui_env->Number);
    }

    ui_env->Number = 1;

    //if (KEY_F(1) == ui_env->Ch) break; /* ESC */ 

    wrefresh(ui_rootuiWindow->Win);
}

UIMap *UIParseMap(char *mapJSON) {
    const struct json_token *tok, *tok2;
    char tmpchar[64];
    int loopJ, x, y, poi, _m;

    UIMap *map = zmalloc(sizeof(map));;
    map->root_json_content = mapJSON;
    map->root_json_tok = parse_json2(mapJSON, strlen(mapJSON));

    tok = find_json_token(map->root_json_tok, "map_width");
    jsonTokToNumber(map->Width, tok, tmpchar);
    tok = find_json_token(map->root_json_tok, "map_height");
    jsonTokToNumber(map->Height, tok, tmpchar);

    tok = find_json_token(map->root_json_tok, "resourses_len");
    jsonTokToNumber(map->resourses_len, tok, tmpchar);

    tok = find_json_token(map->root_json_tok, "resourses");
    map->resourses = (UIMapResourse*)zmalloc(sizeof(UIMapResourse) * map->resourses_len);
    for (loopJ = 0; loopJ < map->resourses_len; loopJ++) {
        sprintf(tmpchar, "resourses[%d].v", loopJ);
        tok2 = find_json_token(map->root_json_tok, tmpchar);
        map->resourses[loopJ].v = *(tok2->ptr);
    }

    tok = find_json_token(map->root_json_tok, "nodes_len");
    jsonTokToNumber(map->nodes_len, tok, tmpchar);

    tok = find_json_token(map->root_json_tok, "nodes");
    g_tmpPtr = zmalloc(sizeof(UIMapNode) * map->Width * map->Height);
    map->nodes = (UIMapNode*)g_tmpPtr;
    memset(map->nodes, 0, sizeof(UIMapNode) * map->Width * map->Height);
    for (loopJ = 0; loopJ < map->nodes_len; loopJ++) {
        sprintf(tmpchar, "nodes[%d][0]", loopJ);
        tok2 = find_json_token(map->root_json_tok, tmpchar);
        jsonTokToNumber(x, tok2, tmpchar);

        sprintf(tmpchar, "nodes[%d][1]", loopJ);
        tok2 = find_json_token(map->root_json_tok, tmpchar);
        jsonTokToNumber(y, tok2, tmpchar);

        poi = MAP_ADDR(x, y, map->Width);
        sprintf(tmpchar, "nodes[%d][2]", loopJ);
        tok2 = find_json_token(map->root_json_tok, tmpchar);
        jsonTokToNumber(_m, tok2, tmpchar);
        map->nodes[poi].resourse = &(map->resourses[_m]);
    }


    /* 画地图需知:
     *  地图左边界、上各留一格空白
     *  地图长或宽小于窗口时，将会居中显示
     */

    map->addr_lt_x = 0;
    map->addr_lt_y = 0;

    if (map->Width > ui_rootuiWindow->Width-1) {
        /* 如果窗口无法一次性显示地图 */
        map->win_lt_x = 1;
        map->win_rb_x = ui_rootuiWindow->Width - 1;
        map->addr_rb_x = (ui_rootuiWindow->Width - 1) - 1;

    } else {
        /* 可以一次性显示，则居中显示 */
        map->win_lt_x = (ui_rootuiWindow->Width - 1) / 2 - map->Width / 2 + 1;
        map->win_rb_x = map->win_lt_x + map->Width - 1;
        map->addr_rb_x = map->Width - 1;
    }


    if (map->Height > ui_rootuiWindow->Height-1) {
        /* 如果窗口无法一次性显示地图 */
        map->win_lt_y = 1;
        map->win_rb_y = ui_rootuiWindow->Height - 1;
        map->addr_rb_y = (ui_rootuiWindow->Height - 1) - 1;

    } else {
        /* 可以一次性显示，则居中显示 */
        map->win_lt_y = (ui_rootuiWindow->Height - 1) / 2 - map->Height / 2 + 1;
        map->win_rb_y = map->win_lt_y + map->Height - 1;
        map->addr_rb_y = map->Height - 1;
    }

    return map;
}

void UIDrawMap() {
    ui_curUIMap->nodes = g_tmpPtr;
    int x, y;
    int poi, _x, _y;
    int loopJ;
    int ch;

    /* 到达左边界 */
    ch = ' ';
    if (0 == ui_curUIMap->addr_lt_x) {
        ch = ACS_VLINE;
    }
    x = ui_curUIMap->win_lt_x - 1;
    y = ui_curUIMap->win_rb_y + 1;
    for (loopJ = ui_curUIMap->win_lt_y-1; loopJ <= y; loopJ++) {
        mvwaddch(ui_rootuiWindow->Win, loopJ, x, ch);
    }

    /* 到达右边界 */
    ch = ' ';
    if (ui_curUIMap->addr_rb_x == ui_curUIMap->Width-1) {
        ch = ACS_VLINE;
    }
    x = ui_curUIMap->win_rb_x + 1;
    y = ui_curUIMap->win_rb_y + 1;
    for (loopJ = ui_curUIMap->win_lt_y-1; loopJ <= y; loopJ++) {
        mvwaddch(ui_rootuiWindow->Win, loopJ, x, ch);
    }

    /* 到达上边界 */
    ch = ' ';
    if (0 == ui_curUIMap->addr_lt_y) {
        ch = ACS_HLINE;
    }
    x = ui_curUIMap->win_rb_x + 1;
    y = ui_curUIMap->win_lt_y - 1;
    for (loopJ = ui_curUIMap->win_lt_x-1; loopJ <= x; loopJ++) {
        mvwaddch(ui_rootuiWindow->Win, y, loopJ, ch);
    }

    /* 到达下边界 */
    ch = ' ';
    if (ui_curUIMap->addr_rb_y == ui_curUIMap->Height-1) {
        ch = ACS_HLINE;
    }
    x = ui_curUIMap->win_rb_x + 1;
    y = ui_curUIMap->win_rb_y + 1;
    for (loopJ = ui_curUIMap->win_lt_x-1; loopJ <= x; loopJ++) {
        mvwaddch(ui_rootuiWindow->Win, y, loopJ, ch);
    }

    /* 四个角处理 */
    /* 左上角 */
    if (0 == ui_curUIMap->addr_lt_x || 0 == ui_curUIMap->addr_lt_y) {
        mvwaddch(ui_rootuiWindow->Win, ui_curUIMap->win_lt_y-1, ui_curUIMap->win_lt_x-1, ACS_ULCORNER);
    }
    /* 左下角 */
    if (0 == ui_curUIMap->addr_lt_x || ui_curUIMap->addr_rb_y == ui_curUIMap->Height-1) {
        mvwaddch(ui_rootuiWindow->Win, ui_curUIMap->win_rb_y+1, ui_curUIMap->win_lt_x-1, ACS_LLCORNER);
    }
    /* 右上角 */
    if (ui_curUIMap->addr_rb_x == ui_curUIMap->Width-1 || 0 == ui_curUIMap->addr_lt_y) {
        mvwaddch(ui_rootuiWindow->Win, ui_curUIMap->win_lt_y-1, ui_curUIMap->win_rb_x+1, ACS_URCORNER);
    }
    /* 右下角 */
    if (ui_curUIMap->addr_rb_x == ui_curUIMap->Width-1 || ui_curUIMap->addr_rb_y == ui_curUIMap->Height-1) {
        mvwaddch(ui_rootuiWindow->Win, ui_curUIMap->win_rb_y+1, ui_curUIMap->win_rb_x+1, ACS_LRCORNER);
    }

    //int x, y; 屏幕上的坐标
    //int poi, _x, _y; 地图坐标
    for (x = ui_curUIMap->win_lt_x; x <= ui_curUIMap->win_rb_x; x++) {
        _x = ui_curUIMap->addr_lt_x + (x - ui_curUIMap->win_lt_x);
        for (y = ui_curUIMap->win_lt_y; y <= ui_curUIMap->win_rb_y; y++) {
            _y = ui_curUIMap->addr_lt_y + (y - ui_curUIMap->win_lt_y);
            poi = MAP_ADDR(_x, _y, ui_curUIMap->Width);
            if (0 == ui_curUIMap->nodes[poi].resourse) {
                mvwaddch(ui_rootuiWindow->Win, y, x, ' ');
            } else {
                mvwaddch(ui_rootuiWindow->Win, y, x, ui_curUIMap->nodes[poi].resourse->v);
            }
        }
    }
    //C_UtilLogI("0x%8X 0x%8X", (unsigned int)ui_curUIMap, (unsigned int)ui_curUIMap->nodes);

    return;
}

void UIMoveCurMapX(int x) {
    int _x = x;
    if (ui_curUIMap->addr_lt_x + x < 0) {
        _x = -1 * ui_curUIMap->addr_lt_x;
    } else if (ui_curUIMap->addr_rb_x + x >= ui_curUIMap->Width-1) {
        _x = ui_curUIMap->Width - ui_curUIMap->addr_rb_x - 1;
    }
    //if (0 == _x) return;
    ui_curUIMap->addr_lt_x += _x;
    ui_curUIMap->addr_rb_x += _x;
    UIDrawMap();
}

void UIMoveCurMapY(int y) {
    int _y = y;
    if (ui_curUIMap->addr_lt_y + y < 0) {
        /* 到达上边界 */
        _y = -1 * ui_curUIMap->addr_lt_y;

    } else if (ui_curUIMap->addr_rb_y + y >= ui_curUIMap->Height-1) {
        /* 到达下边界 */
        _y = ui_curUIMap->Height - ui_curUIMap->addr_rb_y - 1;
    }
    //if (0 == _y) return;
    ui_curUIMap->addr_lt_y += _y;
    ui_curUIMap->addr_rb_y += _y;
    UIDrawMap();
}

void UIFreeUIMap(UIMap *map) {
    zfree(map->root_json_content);
    zfree(map->root_json_tok);

    zfree(map->resourses);
    zfree(map->nodes);
    zfree(map);
}

UIMapNode* UIGetMapNodeByXY(int x, int y) {
    return 0;
}

void UI_InitMap() {
    sds mapJSON;
    char dir[ALLOW_PATH_SIZE] = {""};

    ui_rootuiWindow = UI_createWindow(ui_height-5, ui_width, 0, 0);

    getmaxyx(stdscr, ui_rootuiWindow->Height, ui_rootuiWindow->Width);
    //ui_rootuiWindow->Height--; /* 最后一行不可写 */
    //ui_rootuiWindow->Width--; /* 最后一列不可写 */
    keypad(ui_rootuiWindow->Win, TRUE);

    /* 画首幅地图 */
    //sprintf(dir, "%s/arctic.map.json", m_galaxiesdir);
    sprintf(dir, "/Users/j/github/my/traveller/galaxies/gemini/client/arctic.map.json");
    mapJSON = fileGetContent(dir);
    ui_curUIMap = UIParseMap(mapJSON);

    UIDrawMap();

    /* 光标置中 */
    ui_env->CursorX = ui_rootuiWindow->Width / 2;
    ui_env->CursorY = ui_rootuiWindow->Height / 2;
    wmove(ui_rootuiWindow->Win, ui_env->CursorY, ui_env->CursorX);

    refresh();
    wrefresh(ui_rootuiWindow->Win);

    UI_SubscribeKeyDownEvent((UIKeyDownProcessor)keyDownProcessor);
}
