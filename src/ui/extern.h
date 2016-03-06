#ifndef __UI_EXTERN_H
#define __UI_EXTERN_H

extern uiWindow_t   *ui_rootuiWindow;
extern UIMap        *ui_curUIMap;

extern uiEnv_t      *ui_env;
extern list         *ui_panels;
extern uiConsole_t  *ui_console;
extern int          ui_width, ui_height; //屏幕宽度、高度
extern list         *ui_pages;
extern uiPage_t     *ui_activePage;

extern int          ui_ColorPair[8][8];

extern etDevice_t   *ui_device;

#endif
