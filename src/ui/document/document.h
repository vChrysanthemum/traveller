#ifndef __UI_UI_DOCUMENT_H
#define __UI_UI_DOCUMENT_H

/**
 * html相关
 */
void UIPrepareHtml();

#define UIHTML_TOKEN_START_TAG          0 // <tag>
#define UIHTML_TOKEN_TEXT               1 // 标记中的字符串
#define UIHTML_TOKEN_END_TAG            2 // </tag>
#define UIHTML_TOKEN_SELF_CLOSING_TAG   3 // <tag />

#define UIIsWhiteSpace(c) (' ' == c || '\t' == c || '\r' == c || '\n' == c)

typedef struct {
    char *name;
} UIHtmlDomType;

typedef struct {
    int  type;
    sds  content;
} UIHtmlToken;
void UIHtmlFreeToken(UIHtmlToken *token);
UIHtmlToken* UIHtmlNextToken(char **ptr);

typedef struct UIHtmlDom UIHtmlDom;
typedef struct UIHtmlDom {
    char        *type;
    sds         title;
    dict        *attribute;
    sds         id;
    list        *classes;
    sds         content;
    UIHtmlDom   *parentDom;
    list        *children;
} UIHtmlDom;
UIHtmlDom* UINewHtmlDom();
void UIFreeHtmlDom(void *_dom);
UIHtmlDom* UIParseHtml(char *html);

void UIHtmlPrintDomTree(UIHtmlDom *dom, int indent);

typedef struct {
    sds   content;
    int   width;
    int   height;
    list  *children;
} UIRenderObject;


/**
 * css相关
 */


#endif
