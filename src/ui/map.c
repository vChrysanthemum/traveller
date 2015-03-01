#include <string.h>
#include <ncurses.h>

#include "core/zmalloc.h"
#include "core/util.h"
#include "core/frozen.h"
#include "ui/ui.h"
#include "ui/map.h"

extern UIWin *g_rootUIWin;
UIMap *m_curUIMap;
extern UICursor *g_cursor;

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
    map->nodes = (UIMapNode*)zmalloc(sizeof(UIMapNode) * map->width * map->height);
    memset(map->nodes, 0x00, map->width * map->height);
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

    /* 如果窗口无法一次性显示地图 */
    if (map->width > g_rootUIWin->width-1) {
        map->win_lt_x = 1;
        map->win_rb_x = g_rootUIWin->width - 1;
        map->addr_rb_x = (g_rootUIWin->width - 1) - 1;
    }
    /* 可以一次性显示，则居中显示 */
    else {
        map->win_lt_x = (g_rootUIWin->width - 1) / 2 - map->width / 2 + 1;
        map->win_rb_x = map->win_lt_x + map->width - 1;
        map->addr_rb_x = map->width - 1;
    }


    /* 如果窗口无法一次性显示地图 */
    if (map->height > g_rootUIWin->height-1) {
        map->win_lt_y = 1;
        map->win_rb_y = g_rootUIWin->height - 1;
        map->addr_rb_y = (g_rootUIWin->height - 1) - 1;
    }
    /* 可以一次性显示，则居中显示 */
    else {
        map->win_lt_y = (g_rootUIWin->height - 1) / 2 - map->height / 2 + 1;
        map->win_rb_y = map->win_lt_y + map->height - 1;
        map->addr_rb_y = map->height - 1;
    }

    return map;
}

/* 画地图 */
void UIDrawMap(UIMap *map) {
    m_curUIMap = map;
    int x, y;
    int poi, _x, _y;
    int loopJ;
    int ch;

    /* 到达左边界 */
    ch = ' ';
    if (0 == map->addr_lt_x) {
        ch = ACS_VLINE;
    }
    x = map->win_lt_x - 1;
    y = map->win_rb_y + 1;
    for (loopJ = map->win_lt_y-1; loopJ <= y; loopJ++) {
        mvaddch(loopJ, x, ch);
    }

    /* 到达右边界 */
    ch = ' ';
    if (map->addr_rb_x == map->width-1) {
        ch = ACS_VLINE;
    }
    x = map->win_rb_x + 1;
    y = map->win_rb_y + 1;
    for (loopJ = map->win_lt_y-1; loopJ <= y; loopJ++) {
        mvaddch(loopJ, x, ch);
    }

    /* 到达上边界 */
    ch = ' ';
    if (0 == map->addr_lt_y) {
        ch = ACS_HLINE;
    }
    x = map->win_rb_x + 1;
    y = map->win_lt_y - 1;
    for (loopJ = map->win_lt_x-1; loopJ <= x; loopJ++) {
        mvaddch(y, loopJ, ch);
    }

    /* 到达下边界 */
    ch = ' ';
    if (map->addr_rb_y == map->height-1) {
        ch = ACS_HLINE;
    }
    x = map->win_rb_x + 1;
    y = map->win_rb_y + 1;
    for (loopJ = map->win_lt_x-1; loopJ <= x; loopJ++) {
        mvaddch(y, loopJ, ch);
    }

    /* 四个角处理 */
    /* 左上角 */
    if (0 == map->addr_lt_x || 0 == map->addr_lt_y) {
        mvaddch(map->win_lt_y-1, map->win_lt_x-1, ACS_ULCORNER);
    }
    /* 左下角 */
    if (0 == map->addr_lt_x || map->addr_rb_y == map->height-1) {
        mvaddch(map->win_rb_y+1, map->win_lt_x-1, ACS_LLCORNER);
    }
    /* 右上角 */
    if (map->addr_rb_x == map->width-1 || 0 == map->addr_lt_y) {
        mvaddch(map->win_lt_y-1, map->win_rb_x+1, ACS_URCORNER);
    }
    /* 右下角 */
    if (map->addr_rb_x == map->width-1 || map->addr_rb_y == map->height-1) {
        mvaddch(map->win_rb_y+1, map->win_rb_x+1, ACS_LRCORNER);
    }


    //int x, y; 屏幕上的坐标
    //int poi, _x, _y; 地图坐标
    for (x = map->win_lt_x; x <= map->win_rb_x; x++) {
        _x = map->addr_lt_x + (x - map->win_lt_x);
        for (y = map->win_lt_y; y <= map->win_rb_y; y++) {
            _y = map->addr_lt_y + (y - map->win_lt_y);
            poi = MAP_ADDR(_x, _y, map->width);

            if (NULL == map->nodes[poi].resourse) {
                mvaddch(y, x, ' ');
            }
            else {
                mvaddch(y, x, map->nodes[poi].resourse->v);
            }
        }
    }

    refresh();
}

/* 在x轴上移动地图 */
void UIMoveCurMapX(int x) {
    int _x = x;
    if (m_curUIMap->addr_lt_x + x < 0) {
        _x = -1 * m_curUIMap->addr_lt_x;
    }
    else if (m_curUIMap->addr_rb_x + x >= m_curUIMap->width-1) {
        _x = m_curUIMap->width - m_curUIMap->addr_rb_x - 1;
    }
    //if (0 == _x) return;
    m_curUIMap->addr_lt_x += _x;
    m_curUIMap->addr_rb_x += _x;
    UIDrawMap(m_curUIMap);
}

/* 在y轴上移动地图 */
void UIMoveCurMapY(int y) {
    int _y = y;
    /* 到达上边界 */
    if (m_curUIMap->addr_lt_y + y < 0) {
        _y = -1 * m_curUIMap->addr_lt_y;
    }
    /* 到达下边界 */
    else if (m_curUIMap->addr_rb_y + y >= m_curUIMap->height-1) {
        _y = m_curUIMap->height - m_curUIMap->addr_rb_y - 1;
    }
    //if (0 == _y) return;
    m_curUIMap->addr_lt_y += _y;
    m_curUIMap->addr_rb_y += _y;
    UIDrawMap(m_curUIMap);
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
