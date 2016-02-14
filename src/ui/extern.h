#ifndef __UI_EXTERN_H
#define __UI_EXTERN_H

extern UIWindow *ui_rootUIWindow;
extern UIMap *ui_curUIMap;

extern UIEnv *ui_env;
extern list *ui_panels;
extern UIConsole *ui_console;
extern int ui_width, ui_height; //屏幕宽度、高度
extern list *ui_pages;
extern UIPage *ui_activePage;

extern const int UIHTML_DOM_TEXT;
extern const int UIHTML_DOM_UNKNOWN;
extern const int UIHTML_DOM_HTML;
extern const int UIHTML_DOM_HEAD;
extern const int UIHTML_DOM_TITLE;
extern const int UIHTML_DOM_BODY;
extern const int UIHTML_DOM_SCRIPT;
extern const int UIHTML_DOM_DIV;
extern const int UIHTML_DOM_TABLE;
extern const int UIHTML_DOM_TR;
extern const int UIHTML_DOM_TD;
extern const int UIHTML_DOM_STYLE;

extern dict *UIHtmlDomTypeTable;

extern const int UICOLOR_BLACK; 
extern const int UICOLOR_RED;
extern const int UICOLOR_GREEN;
extern const int UICOLOR_YELLOW;
extern const int UICOLOR_BLUE;
extern const int UICOLOR_MAGENTA;
extern const int UICOLOR_CYAN;
extern const int UICOLOR_WHITE;

extern int UIColorPair[8][8];
extern dict *UIColorPairTable;

extern dict *UIHtmlSpecialStringTable;

extern ETDevice *ui_device;

#endif
