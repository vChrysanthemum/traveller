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

#define UIMoveuiCursor_tLeft(count) do {\
    ui_env->CursorX -= count;\
    if (ui_env->CursorX < 0) {\
        UIMoveCurMapX(ui_env->CursorX);\
        ui_env->CursorX = 0;\
    }\
    wmove(ui_rootuiWindow->Win, ui_env->CursorY, ui_env->CursorX);\
} while (0);

#define UIMoveuiCursor_tRight(count) do {\
    ui_env->CursorX += count;\
    if (ui_env->CursorX > ui_rootuiWindow->Width) {\
        UIMoveCurMapX(ui_env->CursorX - ui_rootuiWindow->Width);\
        ui_env->CursorX = ui_rootuiWindow->Width;\
    }\
    wmove(ui_rootuiWindow->Win, ui_env->CursorY, ui_env->CursorX);\
} while (0);

#define UI_MoveuiCursorUp(count) do {\
    ui_env->CursorY -= count;\
    if (ui_env->CursorY < 0) {\
        UIMoveCurMapY(ui_env->CursorY);\
        ui_env->CursorY = 0;\
    }\
    wmove(ui_rootuiWindow->Win, ui_env->CursorY, ui_env->CursorX);\
} while (0);

#define UIMoveuiCursor_tDown(count) do {\
    ui_env->CursorY += count;\
    if (ui_env->CursorY > ui_rootuiWindow->Height) {\
        UIMoveCurMapY(ui_env->CursorY - ui_rootuiWindow->Height);\
        ui_env->CursorY = ui_rootuiWindow->Height;\
    }\
    wmove(ui_rootuiWindow->Win, ui_env->CursorY, ui_env->CursorX);\
} while (0);

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
    int Width;
    int Height;

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
