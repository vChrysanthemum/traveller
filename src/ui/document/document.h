#ifndef __UI_UI_DOCUMENT_H
#define __UI_UI_DOCUMENT_H

#include "core/adlist.h"

typedef struct {
    int  type;
    sds  content;
} uiDocumentScanToken_t;
uiDocumentScanToken_t* UI_NewDocumentScanToken();
void UI_FreeDocumentScanToken(uiDocumentScanToken_t *token);

typedef struct uiDocumentScanner_t uiDocumentScanner_t;
typedef struct uiDocumentScanner_t {
    char *content;
    char *current;
    uiDocumentScanToken_t* (*scan) (uiDocumentScanner_t *scanner);
} uiDocumentScanner_t;

/**
 * html 相关
 */
void UI_PrepareHtml();

#define UIHTML_TOKEN_TEXT               -1 // 标记中的字符串
#define UIHTML_TOKEN_START_TAG          0  // <tag>
#define UIHTML_TOKEN_END_TAG            1  // </tag>
#define UIHTML_TOKEN_SELF_CLOSING_TAG   2  // <tag />

#define UIIsWhiteSpace(c) (' ' == c || '\t' == c || '\r' == c || '\n' == c)

uiDocumentScanToken_t* UI_ScanHtmlToken(uiDocumentScanner_t *scanner);

typedef struct {
    char *name;
    enum uiHtmlDomType_e {
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
} uiHtmlDomInfo_t;

typedef struct uiHtmlDom_t uiHtmlDom_t;
typedef struct uiHtmlDom_t {
    sds         title;
    dict        *attribute;
    sds         id;
    list        *classes;
    sds         content;
    uiHtmlDom_t   *parentDom;
    list        *children;
    enum uiHtmlDomType_e type;
} uiHtmlDom_t;
uiHtmlDom_t* UI_NewHtmlDom();
void UI_FreeHtmlDom(void *_dom);
uiHtmlDom_t* UI_ParseHtml(char *html);

void UI_PrintHtmlDomTree(uiHtmlDom_t *dom, int indent);

/**
 * css 相关
 */
void UI_PrepareCss();

typedef struct {
    char *name;
    enum uiCssPropertyType_e {
        UICSS_PROPERTY_TYPE_BACKGROUND_COLOR,
        UICSS_PROPERTY_TYPE_COLOR,
        UICSS_PROPERTY_TYPE_PADDING,
        UICSS_PROPERTY_TYPE_MARGIN,
        UICSS_PROPERTY_TYPE_DISPLAY,
        UICSS_PROPERTY_TYPE_TEXT_ALIGN,
    } type;
} uiCssPropertyInfo_t;

typedef struct {
    enum uiCssPropertyType_e type;
} uiCssProperty_t;

typedef struct {
} uiCssSelector_t;

typedef struct {
    list *selectors;
} uiCssRule_t;

typedef struct {
    list * rulers;
} uiCssStyleSheet_t;
uiCssStyleSheet_t* UI_ParseCssStyleSheet(char *css);

typedef struct uiCssObject_t uiCssObject_t;
typedef struct uiCssObject_t {
    uiCssProperty_t *property;
} uiCssObject_t;


/**
 * render 相关
 */
typedef struct {
    sds   content;
    int   width;
    int   height;
    list  *children;
} uiRenderObject_t;

#endif
