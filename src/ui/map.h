#ifndef __UI_MAP_H
#define __UI_MAP_H

#include "core/frozen.h"

/* 根据x,y，返回map上节点键值 */
#define MAP_ADDR(x, y, width) (y * width + x)


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

    /* 地图在窗口上显示左上角位置 */
    int win_lt_x;
    int win_lt_y;

    /* 地图在窗口上显示右上角位置 */
    int win_rb_x;
    int win_rb_y;

    /* 窗口上地图左上角，相对应地图上的坐标 */
    int addr_lt_x;
    int addr_lt_y;

    /* 窗口上地图右下角，相对应地图上的坐标 */
    int addr_rb_x;
    int addr_rb_y;
} UIMap;

UIMap *UIParseMap(char *mapJSON);
void UIDrawMap(UIMap *map);
void UIMoveCurMapX(int x);
void UIMoveCurMapY(int y);
void UIFreeUIMap(UIMap *map);
UIMapNode* UIGetMapNodeByXY(int x, int y);
#endif
