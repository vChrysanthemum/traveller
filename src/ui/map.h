#ifndef __UI_MAP_H
#define __UI_MAP_H

#include "core/frozen.h"

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
 */

/* 根据x,y，返回map上节点键值 */
#define MAP_ADDR(x, y, width) (y * width + x)

#define UIMoveUICursorLeft(count) do {\
    ui_env->cursor_x -= count;\
    if (ui_env->cursor_x < 0) {\
        UIMoveCurMapX(ui_env->cursor_x);\
        ui_env->cursor_x = 0;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorRight(count) do {\
    ui_env->cursor_x += count;\
    if (ui_env->cursor_x > ui_rootUIWindow->width) {\
        UIMoveCurMapX(ui_env->cursor_x - ui_rootUIWindow->width);\
        ui_env->cursor_x = ui_rootUIWindow->width;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorUp(count) do {\
    ui_env->cursor_y -= count;\
    if (ui_env->cursor_y < 0) {\
        UIMoveCurMapY(ui_env->cursor_y);\
        ui_env->cursor_y = 0;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

#define UIMoveUICursorDown(count) do {\
    ui_env->cursor_y += count;\
    if (ui_env->cursor_y > ui_rootUIWindow->height) {\
        UIMoveCurMapY(ui_env->cursor_y - ui_rootUIWindow->height);\
        ui_env->cursor_y = ui_rootUIWindow->height;\
    }\
    wmove(ui_rootUIWindow->win, ui_env->cursor_y, ui_env->cursor_x);\
} while(0);

/* 地图上的物体 */
typedef struct {
    char v;                 /* 显示出来的字符 */
} UIMapStuff;


/* 继承以下结构：
 *  UIMapStuff
 */
typedef struct {
    char v;                 /* 显示出来的字符 */
} UIGamer;


/* 继承以下结构：
 *  UIMapStuff
 */
typedef struct {
    char v;                 /* 显示出来的字符 */
    char *name;
    int name_len;
    char *introduction;
    int introduction_len;
    int flag;
} UIMapResourse;

typedef struct {
    UIMapResourse *resourse;
    int position_x;
    int position_y;
} UIMapNode;

typedef struct {
    UIMapResourse *resourses;
    int resourses_len;
    UIMapNode *nodes;
    int nodes_len;
    struct json_token *root_json_tok;
    char *root_json_content;
    int width;
    int height;

    /* 地图在窗口上显示左上角位置，一次生成，不再修改 */
    int win_lt_x;
    int win_lt_y;

    /* 地图在窗口上显示右上角位置，一次生成，不再修改 */
    int win_rb_x;
    int win_rb_y;

    /* 窗口上地图左上角，相对应地图上的坐标 */
    int addr_lt_x;
    int addr_lt_y;

    /* 窗口上地图右下角，相对应地图上的坐标 */
    int addr_rb_x;
    int addr_rb_y;
} UIMap;

/* 将字符串转换成地图 */
UIMap *UIParseMap(char *mapJSON);
/* 画地图 */
void UIDrawMap();
/* 在x轴上移动地图 */
void UIMoveCurMapX(int x);
/* 在y轴上移动地图 */
void UIMoveCurMapY(int y);
/* 释放地图 */
void UIFreeUIMap(UIMap *map);
/* 根据窗口x、y，获取相对应的地图节点 */
UIMapNode* UIGetMapNodeByXY(int x, int y);
#endif
