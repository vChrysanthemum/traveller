#ifndef __UI_UI_DOCUMENT_H
#define __UI_UI_DOCUMENT_H

/**
 * html相关
 */
void UIPrepareHtml();

#define UIHTML_TOKEN_TEXT               -1 // 标记中的字符串
#define UIHTML_TOKEN_START_TAG          0  // <tag>
#define UIHTML_TOKEN_END_TAG            1  // </tag>
#define UIHTML_TOKEN_SELF_CLOSING_TAG   2  // <tag />

#define UIIsWhiteSpace(c) (' ' == c || '\t' == c || '\r' == c || '\n' == c)


typedef struct {
    char *name;
    enum UIHtmlDomType {
        UIHTML_DOM_TYPE_UNKNOWN,
        UIHTML_DOM_TYPE_TEXT,
        UIHTML_DOM_TYPE_HTML,
        UIHTML_DOM_TYPE_HEAD,
        UIHTML_DOM_TYPE_TITLE,
        UIHTML_DOM_TYPE_BODY,
        UIHTML_DOM_TYPE_SCRIPT,
        UIHTML_DOM_TYPE_DIV,
        UIHTML_DOM_TYPE_TABLE,
        UIHTML_DOM_TYPE_TR,
        UIHTML_DOM_TYPE_TD,
        UIHTML_DOM_TYPE_STYLE,
    } type;
} UIHtmlDomInfo;

typedef struct {
    int  type;
    sds  content;
} UIHtmlToken;
void UIHtmlFreeToken(UIHtmlToken *token);
UIHtmlToken* UIHtmlNextToken(char **ptr);

typedef struct UIHtmlDom UIHtmlDom;
typedef struct UIHtmlDom {
    sds         title;
    dict        *attribute;
    sds         id;
    list        *classes;
    sds         content;
    UIHtmlDom   *parentDom;
    list        *children;
    enum UIHtmlDomType type;
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
void UIPrepareCss();

typedef struct {
    char *name;
    enum UICssPropertyType {
        UICSS_PROPERTY_TYPE_BACKGROUND_COLOR,
        UICSS_PROPERTY_TYPE_COLOR,
        UICSS_PROPERTY_TYPE_PADDING,
        UICSS_PROPERTY_TYPE_MARGIN,
        UICSS_PROPERTY_TYPE_DISPLAY,
        UICSS_PROPERTY_TYPE_TEXT_ALIGN,
    } type;
} UICssPropertyInfo;

typedef struct {
    enum UICssPropertyType type;
} UICssProperty;

typedef struct UICssObject UICssObject;
typedef struct UICssObject {
    UICssProperty *property;
} UICssObject;


#endif
