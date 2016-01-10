#include <string.h>

#include "core/zmalloc.h"
#include "core/util.h"
#include "core/frozen.h"
#include "ui/ui.h"
#include "ui/map.h"
#include "extern.h"

extern UIWindow *ui_rootUIWindow;
extern UIMap *ui_curUIMap;

static void *g_tmpPtr;

static void keyDownProcessor (int ch) {
    if ('0' <= ui_env->ch && ui_env->ch <= '9') {
        if (ui_env->snumber_len > 6) {
            return;
        }

        ui_env->snumber[ui_env->snumber_len] = ui_env->ch;
        ui_env->snumber_len++;
        ui_env->snumber[ui_env->snumber_len] = 0;

        return;
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

    //if (KEY_F(1) == ui_env->ch) break; /* ESC */ 

    wrefresh(ui_rootUIWindow->win);
}

UIMap *UIParseMap(char *mapJSON) {
    const struct json_token *tok, *tok2;
    char tmpchar[64];
    int loopJ, x, y, poi, _m;

    UIMap *map = zmalloc(sizeof(map));;
    map->root_json_content = mapJSON;
    map->root_json_tok = parse_json2(mapJSON, strlen(mapJSON));

    tok = find_json_token(map->root_json_tok, "map_width");
    jsonTokToNumber(map->width, tok, tmpchar);
    tok = find_json_token(map->root_json_tok, "map_height");
    jsonTokToNumber(map->height, tok, tmpchar);

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
    g_tmpPtr = zmalloc(sizeof(UIMapNode) * map->width * map->height);
    map->nodes = (UIMapNode*)g_tmpPtr;
    memset(map->nodes, 0, sizeof(UIMapNode) * map->width * map->height);
    for (loopJ = 0; loopJ < map->nodes_len; loopJ++) {
        sprintf(tmpchar, "nodes[%d][0]", loopJ);
        tok2 = find_json_token(map->root_json_tok, tmpchar);
        jsonTokToNumber(x, tok2, tmpchar);

        sprintf(tmpchar, "nodes[%d][1]", loopJ);
        tok2 = find_json_token(map->root_json_tok, tmpchar);
        jsonTokToNumber(y, tok2, tmpchar);

        poi = MAP_ADDR(x, y, map->width);
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

    if (map->width > ui_rootUIWindow->width-1) {
        /* 如果窗口无法一次性显示地图 */
        map->win_lt_x = 1;
        map->win_rb_x = ui_rootUIWindow->width - 1;
        map->addr_rb_x = (ui_rootUIWindow->width - 1) - 1;

    } else {
        /* 可以一次性显示，则居中显示 */
        map->win_lt_x = (ui_rootUIWindow->width - 1) / 2 - map->width / 2 + 1;
        map->win_rb_x = map->win_lt_x + map->width - 1;
        map->addr_rb_x = map->width - 1;
    }


    if (map->height > ui_rootUIWindow->height-1) {
        /* 如果窗口无法一次性显示地图 */
        map->win_lt_y = 1;
        map->win_rb_y = ui_rootUIWindow->height - 1;
        map->addr_rb_y = (ui_rootUIWindow->height - 1) - 1;

    } else {
        /* 可以一次性显示，则居中显示 */
        map->win_lt_y = (ui_rootUIWindow->height - 1) / 2 - map->height / 2 + 1;
        map->win_rb_y = map->win_lt_y + map->height - 1;
        map->addr_rb_y = map->height - 1;
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
        mvwaddch(ui_rootUIWindow->win, loopJ, x, ch);
    }

    /* 到达右边界 */
    ch = ' ';
    if (ui_curUIMap->addr_rb_x == ui_curUIMap->width-1) {
        ch = ACS_VLINE;
    }
    x = ui_curUIMap->win_rb_x + 1;
    y = ui_curUIMap->win_rb_y + 1;
    for (loopJ = ui_curUIMap->win_lt_y-1; loopJ <= y; loopJ++) {
        mvwaddch(ui_rootUIWindow->win, loopJ, x, ch);
    }

    /* 到达上边界 */
    ch = ' ';
    if (0 == ui_curUIMap->addr_lt_y) {
        ch = ACS_HLINE;
    }
    x = ui_curUIMap->win_rb_x + 1;
    y = ui_curUIMap->win_lt_y - 1;
    for (loopJ = ui_curUIMap->win_lt_x-1; loopJ <= x; loopJ++) {
        mvwaddch(ui_rootUIWindow->win, y, loopJ, ch);
    }

    /* 到达下边界 */
    ch = ' ';
    if (ui_curUIMap->addr_rb_y == ui_curUIMap->height-1) {
        ch = ACS_HLINE;
    }
    x = ui_curUIMap->win_rb_x + 1;
    y = ui_curUIMap->win_rb_y + 1;
    for (loopJ = ui_curUIMap->win_lt_x-1; loopJ <= x; loopJ++) {
        mvwaddch(ui_rootUIWindow->win, y, loopJ, ch);
    }

    /* 四个角处理 */
    /* 左上角 */
    if (0 == ui_curUIMap->addr_lt_x || 0 == ui_curUIMap->addr_lt_y) {
        mvwaddch(ui_rootUIWindow->win, ui_curUIMap->win_lt_y-1, ui_curUIMap->win_lt_x-1, ACS_ULCORNER);
    }
    /* 左下角 */
    if (0 == ui_curUIMap->addr_lt_x || ui_curUIMap->addr_rb_y == ui_curUIMap->height-1) {
        mvwaddch(ui_rootUIWindow->win, ui_curUIMap->win_rb_y+1, ui_curUIMap->win_lt_x-1, ACS_LLCORNER);
    }
    /* 右上角 */
    if (ui_curUIMap->addr_rb_x == ui_curUIMap->width-1 || 0 == ui_curUIMap->addr_lt_y) {
        mvwaddch(ui_rootUIWindow->win, ui_curUIMap->win_lt_y-1, ui_curUIMap->win_rb_x+1, ACS_URCORNER);
    }
    /* 右下角 */
    if (ui_curUIMap->addr_rb_x == ui_curUIMap->width-1 || ui_curUIMap->addr_rb_y == ui_curUIMap->height-1) {
        mvwaddch(ui_rootUIWindow->win, ui_curUIMap->win_rb_y+1, ui_curUIMap->win_rb_x+1, ACS_LRCORNER);
    }

    //int x, y; 屏幕上的坐标
    //int poi, _x, _y; 地图坐标
    for (x = ui_curUIMap->win_lt_x; x <= ui_curUIMap->win_rb_x; x++) {
        _x = ui_curUIMap->addr_lt_x + (x - ui_curUIMap->win_lt_x);
        for (y = ui_curUIMap->win_lt_y; y <= ui_curUIMap->win_rb_y; y++) {
            _y = ui_curUIMap->addr_lt_y + (y - ui_curUIMap->win_lt_y);
            poi = MAP_ADDR(_x, _y, ui_curUIMap->width);
            if (0 == ui_curUIMap->nodes[poi].resourse) {
                mvwaddch(ui_rootUIWindow->win, y, x, ' ');
            } else {
                mvwaddch(ui_rootUIWindow->win, y, x, ui_curUIMap->nodes[poi].resourse->v);
            }
        }
    }
    //TrvLogI("0x%8X 0x%8X", (unsigned int)ui_curUIMap, (unsigned int)ui_curUIMap->nodes);

    return;
}

void UIMoveCurMapX(int x) {
    int _x = x;
    if (ui_curUIMap->addr_lt_x + x < 0) {
        _x = -1 * ui_curUIMap->addr_lt_x;
    }
    else if (ui_curUIMap->addr_rb_x + x >= ui_curUIMap->width-1) {
        _x = ui_curUIMap->width - ui_curUIMap->addr_rb_x - 1;
    }
    //if (0 == _x) return;
    ui_curUIMap->addr_lt_x += _x;
    ui_curUIMap->addr_rb_x += _x;
    UIDrawMap();
}

void UIMoveCurMapY(int y) {
    int _y = y;
    /* 到达上边界 */
    if (ui_curUIMap->addr_lt_y + y < 0) {
        _y = -1 * ui_curUIMap->addr_lt_y;
    }
    /* 到达下边界 */
    else if (ui_curUIMap->addr_rb_y + y >= ui_curUIMap->height-1) {
        _y = ui_curUIMap->height - ui_curUIMap->addr_rb_y - 1;
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
    return NULL;
}

void UIInitMap() {
    sds mapJSON;
    char dir[ALLOW_PATH_SIZE] = {""};

    ui_rootUIWindow = UIcreateWindow(ui_height-5, ui_width, 0, 0);

    getmaxyx(stdscr, ui_rootUIWindow->height, ui_rootUIWindow->width);
    //ui_rootUIWindow->height--; /* 最后一行不可写 */
    //ui_rootUIWindow->width--; /* 最后一列不可写 */
    keypad(ui_rootUIWindow->win, TRUE);

    /* 画首幅地图 */
    //sprintf(dir, "%s/arctic.map.json", m_galaxiesdir);
    sprintf(dir, "/Users/j/github/my/traveller/galaxies/gemini/client/arctic.map.json");
    mapJSON = fileGetContent(dir);
    ui_curUIMap = UIParseMap(mapJSON);

    UIDrawMap();

    /* 光标置中 */
    ui_env->cursor_x = ui_rootUIWindow->width / 2;
    ui_env->cursor_y = ui_rootUIWindow->height / 2;
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);

    refresh();
    wrefresh(ui_rootUIWindow->win);

    UISubscribeKeyDownEvent((UIKeyDownProcessor)keyDownProcessor);
}
