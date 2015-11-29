#include <string.h>
#include <ncurses.h>

#include "core/zmalloc.h"
#include "core/util.h"
#include "core/frozen.h"
#include "ui/ui.h"
#include "ui/map.h"

extern UIWin *g_rootUIWin;
extern UICursor *g_cursor;
extern UIMap *g_curUIMap;
extern void *g_tmpPtr;

/* 地图json格式：
 * {
 *   'map_width' : 300, //地图宽
 *   'map_height': 300, //地图长
 *   'resourses' : {
 *      [1] : {
 *          'v'     : '&', // 显示出来的字符
 *          'name'  : '岩石',
 *          'introduction' : '无法移动的岩石',
 *          'is_overlay'   : 0  // 该点上面是否能叠加物体，譬如玩家是否可以站在这上面 
 *      },
 *      ...
 *   }
 *   'nodes' : [
 *      [$position_x, $position_y, $resourse_id],
 *      ...
 *   ]
 * }
 * 
 */


/* 将字符串转换成地图 
 */
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

    if (map->width > g_rootUIWin->width-1) {
        /* 如果窗口无法一次性显示地图 */
        map->win_lt_x = 1;
        map->win_rb_x = g_rootUIWin->width - 1;
        map->addr_rb_x = (g_rootUIWin->width - 1) - 1;

    } else {
        /* 可以一次性显示，则居中显示 */
        map->win_lt_x = (g_rootUIWin->width - 1) / 2 - map->width / 2 + 1;
        map->win_rb_x = map->win_lt_x + map->width - 1;
        map->addr_rb_x = map->width - 1;
    }


    if (map->height > g_rootUIWin->height-1) {
        /* 如果窗口无法一次性显示地图 */
        map->win_lt_y = 1;
        map->win_rb_y = g_rootUIWin->height - 1;
        map->addr_rb_y = (g_rootUIWin->height - 1) - 1;

    } else {
        /* 可以一次性显示，则居中显示 */
        map->win_lt_y = (g_rootUIWin->height - 1) / 2 - map->height / 2 + 1;
        map->win_rb_y = map->win_lt_y + map->height - 1;
        map->addr_rb_y = map->height - 1;
    }

    return map;
}

/* 画地图 */
void UIDrawMap() {
    g_curUIMap->nodes = g_tmpPtr;
    int x, y;
    int poi, _x, _y;
    int loopJ;
    int ch;

    /* 到达左边界 */
    ch = ' ';
    if (0 == g_curUIMap->addr_lt_x) {
        ch = ACS_VLINE;
    }
    x = g_curUIMap->win_lt_x - 1;
    y = g_curUIMap->win_rb_y + 1;
    for (loopJ = g_curUIMap->win_lt_y-1; loopJ <= y; loopJ++) {
        mvwaddch(g_rootUIWin->window, loopJ, x, ch);
    }

    /* 到达右边界 */
    ch = ' ';
    if (g_curUIMap->addr_rb_x == g_curUIMap->width-1) {
        ch = ACS_VLINE;
    }
    x = g_curUIMap->win_rb_x + 1;
    y = g_curUIMap->win_rb_y + 1;
    for (loopJ = g_curUIMap->win_lt_y-1; loopJ <= y; loopJ++) {
        mvwaddch(g_rootUIWin->window, loopJ, x, ch);
    }

    /* 到达上边界 */
    ch = ' ';
    if (0 == g_curUIMap->addr_lt_y) {
        ch = ACS_HLINE;
    }
    x = g_curUIMap->win_rb_x + 1;
    y = g_curUIMap->win_lt_y - 1;
    for (loopJ = g_curUIMap->win_lt_x-1; loopJ <= x; loopJ++) {
        mvwaddch(g_rootUIWin->window, y, loopJ, ch);
    }

    /* 到达下边界 */
    ch = ' ';
    if (g_curUIMap->addr_rb_y == g_curUIMap->height-1) {
        ch = ACS_HLINE;
    }
    x = g_curUIMap->win_rb_x + 1;
    y = g_curUIMap->win_rb_y + 1;
    for (loopJ = g_curUIMap->win_lt_x-1; loopJ <= x; loopJ++) {
        mvwaddch(g_rootUIWin->window, y, loopJ, ch);
    }

    /* 四个角处理 */
    /* 左上角 */
    if (0 == g_curUIMap->addr_lt_x || 0 == g_curUIMap->addr_lt_y) {
        mvwaddch(g_rootUIWin->window, g_curUIMap->win_lt_y-1, g_curUIMap->win_lt_x-1, ACS_ULCORNER);
    }
    /* 左下角 */
    if (0 == g_curUIMap->addr_lt_x || g_curUIMap->addr_rb_y == g_curUIMap->height-1) {
        mvwaddch(g_rootUIWin->window, g_curUIMap->win_rb_y+1, g_curUIMap->win_lt_x-1, ACS_LLCORNER);
    }
    /* 右上角 */
    if (g_curUIMap->addr_rb_x == g_curUIMap->width-1 || 0 == g_curUIMap->addr_lt_y) {
        mvwaddch(g_rootUIWin->window, g_curUIMap->win_lt_y-1, g_curUIMap->win_rb_x+1, ACS_URCORNER);
    }
    /* 右下角 */
    if (g_curUIMap->addr_rb_x == g_curUIMap->width-1 || g_curUIMap->addr_rb_y == g_curUIMap->height-1) {
        mvwaddch(g_rootUIWin->window, g_curUIMap->win_rb_y+1, g_curUIMap->win_rb_x+1, ACS_LRCORNER);
    }

    //int x, y; 屏幕上的坐标
    //int poi, _x, _y; 地图坐标
    for (x = g_curUIMap->win_lt_x; x <= g_curUIMap->win_rb_x; x++) {
        _x = g_curUIMap->addr_lt_x + (x - g_curUIMap->win_lt_x);
        for (y = g_curUIMap->win_lt_y; y <= g_curUIMap->win_rb_y; y++) {
            _y = g_curUIMap->addr_lt_y + (y - g_curUIMap->win_lt_y);
            poi = MAP_ADDR(_x, _y, g_curUIMap->width);
            if (0 == g_curUIMap->nodes[poi].resourse) {
                mvwaddch(g_rootUIWin->window, y, x, ' ');
            } else {
                mvwaddch(g_rootUIWin->window, y, x, g_curUIMap->nodes[poi].resourse->v);
            }
        }
    }
    ZeusLogI("0x%8X 0x%8X", (unsigned int)g_curUIMap, (unsigned int)g_curUIMap->nodes);

    return;
}

/* 在x轴上移动地图 */
void UIMoveCurMapX(int x) {
    int _x = x;
    if (g_curUIMap->addr_lt_x + x < 0) {
        _x = -1 * g_curUIMap->addr_lt_x;
    }
    else if (g_curUIMap->addr_rb_x + x >= g_curUIMap->width-1) {
        _x = g_curUIMap->width - g_curUIMap->addr_rb_x - 1;
    }
    //if (0 == _x) return;
    g_curUIMap->addr_lt_x += _x;
    g_curUIMap->addr_rb_x += _x;
    UIDrawMap();
}

/* 在y轴上移动地图 */
void UIMoveCurMapY(int y) {
    int _y = y;
    /* 到达上边界 */
    if (g_curUIMap->addr_lt_y + y < 0) {
        _y = -1 * g_curUIMap->addr_lt_y;
    }
    /* 到达下边界 */
    else if (g_curUIMap->addr_rb_y + y >= g_curUIMap->height-1) {
        _y = g_curUIMap->height - g_curUIMap->addr_rb_y - 1;
    }
    //if (0 == _y) return;
    g_curUIMap->addr_lt_y += _y;
    g_curUIMap->addr_rb_y += _y;
    UIDrawMap();
}

/* 释放地图 */
void UIFreeUIMap(UIMap *map) {
    zfree(map->root_json_content);
    zfree(map->root_json_tok);

    zfree(map->resourses);
    zfree(map->nodes);
    zfree(map);
}

/* 移动地图上可移动的物体
 */
void UIMoveStuff(UIMapStuff *stuff) {

}

/* 根据窗口x、y，获取相对应的地图节点 */
UIMapNode* UIGetMapNodeByXY(int x, int y) {
    return NULL;
} 
