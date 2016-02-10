#ifndef __UI_UI_H
#define __UI_UI_H

#include <panel.h>
#include <curses.h>
#include "core/errors.h"
#include "core/util.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/zmalloc.h"
#include "ui/map.h"

//ColorPair
#define CP_CONSOLE_TAB              1
#define CP_CONSOLE_TAB_ACTIVE       2
#define CP_CONSOLE_TAB_BG           3

#define CONSOLE_MODE_CMD 1

#define UI_MAX_PANELS 32;

typedef struct {
    int y;
    int x;
    char             utf8char[4];
    int              utf8charPoi;
} UICursor;

typedef struct {
    int  ch;
    int  cursorY;
    int  cursorX;
    int  number;      //已输入的数字
    char snumber[8];  //已输入的数字
    int  snumberLen;
} UIEnv;

typedef struct {
    int    height; //行数 
    int    width;  //列数 
    int    startx;
    int    starty;
    WINDOW *win;
    PANEL  *panel;
} UIWindow;
UIWindow* UIcreateWindow(int height, int width, int starty, int startx);
void UIFreeWindow(UIWindow* win);

typedef struct {
    sds      title;
    sds      content;
    UIWindow *uiwin;
} UIPage;
UIPage *UINewPage();
void UIFreePage(UIPage *page);
void* UILoadPageActor(ETActor *actor, int args, void **argv);

typedef struct {
    sds line;
    sds header;
} UIConsoleCommand;

typedef struct {
    UIWindow         *tabuiwin;
    UIWindow         *uiwin;
    int              mode;
    UICursor         cursor;
    UIConsoleCommand cmd;
} UIConsole;

int UIPrepare();
int UIInit();

void UIinitColor();

typedef void (*UIKeyDownProcessor) (char ch);
int UISubscribeKeyDownEvent(UIKeyDownProcessor subscriber);
int UIUnSubscribeKeyDownEvent(UIKeyDownProcessor subscriber);

void UIinitConsole();
void UIreRenderConsole();

void UIInitMap();

/**
 * html相关
 */
dict *UIHtmlDomTypeTable;

void UIHtmlPrepare();

#define UIHTML_TOKEN_START_TAG          0 // <tag>
#define UIHTML_TOKEN_TEXT               1 // 标记中的字符串
#define UIHTML_TOKEN_END_TAG            2 // </tag>
#define UIHTML_TOKEN_SELF_CLOSING_TAG   3 // <tag />

#define UIIsWhiteSpace(c) (' ' == c || '\t' == c || '\r' == c || '\n' == c)

typedef struct UIHtmlToken {
    int  type;
    sds  content;
} UIHtmlToken;
void UIHtmlFreeToken(UIHtmlToken *token);
UIHtmlToken* UIHtmlNextToken(char **ptr);

typedef struct UIHtmlDom UIHtmlDom;
typedef struct UIHtmlDom {
    int         type;
    sds         title;
    dict        *attribute;
    sds         content;
    UIHtmlDom   *parentDom;
    list        *children;
} UIHtmlDom;
UIHtmlDom* UINewHtmlDom();
void UIFreeHtmlDom(void *_dom);
UIHtmlDom* UIParseHtml(char *html);

void UIHtmlPrintDomTree(UIHtmlDom *dom, int indent);

typedef struct UIRenderDom {
} UIRenderDom;

#endif
