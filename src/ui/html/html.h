#ifndef __UI_HTML_H
#define __UI_HTML_H

#define HT_ROOT;
#define HT_HEADER;
#define HT_BODY;
#define HT_MENU;
#define HT_MENU_ITEM;
#define HT_DIV;
#define HT_TABLE;
#define HT_TR;
#define HT_TD;

typedef struct UIHTDom {
    int  type;
    void *data;
    list *children;
} UIHTDom;

#endif
