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

extern int UIColorPair[8][8];

extern ETDevice *ui_device;

#endif
