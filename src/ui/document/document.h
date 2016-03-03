#ifndef __UI_UI_DOCUMENT_H
#define __UI_UI_DOCUMENT_H

const char* UIERROR_CSS_PARSE_STATE_NOT_SELECTOR;
const char* UIERROR_CSS_PARSE_STATE_NOT_PROPERTY_KEY;
const char* UIERROR_CSS_PARSE_STATE_NOT_PROPERTY_VALUE;

typedef struct uiDocumentScanToken_s {
    int  type;
    sds  content;
} uiDocumentScanToken_t;
uiDocumentScanToken_t* UI_NewDocumentScanToken();
void UI_FreeDocumentScanToken(uiDocumentScanToken_t *token);

typedef struct uiDocumentScanner_s uiDocumentScanner_t;
typedef struct uiDocumentScanner_s {
    int  state;
    char *content;
    char *current;
    uiDocumentScanToken_t* (*scan) (uiDocumentScanner_t *scanner);
} uiDocumentScanner_t;

typedef struct uiDocument_s uiDocument_t;

/**
 * html 相关
 */
void UI_PrepareHtml();

#define UIHTML_TOKEN_TEXT               -1 // 标记中的字符串
#define UIHTML_TOKEN_START_TAG          -2 // <tag>
#define UIHTML_TOKEN_END_TAG            -3 // </tag>
#define UIHTML_TOKEN_SELF_CLOSING_TAG   -4 // <tag />

#define UI_IsWhiteSpace(c) (' ' == c || '\t' == c || '\r' == c || '\n' == c)

uiDocumentScanToken_t* UI_ScanHtmlToken(uiDocumentScanner_t *scanner);

typedef struct uiHtmlDomInfo_s {
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
        UIHTML_DOM_TYPE_INPUT,
    } type;
} uiHtmlDomInfo_t;

typedef struct uiHtmlDom_s uiHtmlDom_t;
typedef struct uiHtmlDom_s {
    sds         title;
    dict        *attribute;
    sds         id;
    list        *classes;  // list sds
    sds         content;
    uiHtmlDom_t *parent;
    list        *children;
    enum uiHtmlDomType_e type;
} uiHtmlDom_t;
uiHtmlDom_t* UI_NewHtmlDom();
void UI_FreeHtmlDom(void *_dom);
int UI_ParseHtml(uiDocument_t *document);

void UI_PrintHtmlDomTree(uiHtmlDom_t *dom, int indent);

/**
 * css 相关
 */
#define UICSS_PARSE_STATE_SELECTOR         2 // 解析selector阶段
#define UICSS_PARSE_STATE_PROPERTY_KEY     5 // 解析property中的key阶段
#define UICSS_PARSE_STATE_PROPERTY_VALUE   6 // 解析property中的value阶段

#define UICSS_TOKEN_TEXT            '*' // 字符串
#define UICSS_TOKEN_COMMA           ',' // ,
#define UICSS_TOKEN_COLON           ':' // :
#define UICSS_TOKEN_SEMICOLON       ';' // ;
#define UICSS_TOKEN_BLOCK_START     '{' // {
#define UICSS_TOKEN_BLOCK_END       '}' // }

void UI_PrepareCss();

typedef struct uiCssPropertyInfo_s {
    char *name;
    enum uiCssPropertyType_e {
        UICSS_PROPERTY_TYPE_UNKNOWN,
        UICSS_PROPERTY_TYPE_BACKGROUND_COLOR,
        UICSS_PROPERTY_TYPE_COLOR,
        UICSS_PROPERTY_TYPE_PADDING,
        UICSS_PROPERTY_TYPE_MARGIN,
        UICSS_PROPERTY_TYPE_DISPLAY,
        UICSS_PROPERTY_TYPE_TEXT_ALIGN,
        UICSS_PROPERTY_TYPE_WIDTH,
        UICSS_PROPERTY_TYPE_HEIGHT,
    } type;
} uiCssPropertyInfo_t;

typedef struct uiCssProperty_s {
    enum uiCssPropertyType_e type;
    sds key;
    sds value;
} uiCssProperty_t;
uiCssProperty_t* UI_NewCssProperty();
void UI_FreeCssProperty(void *_property);

typedef struct uiCssPropertyList_s {
    int referenceCount;
    list *data;
} uiCssPropertyList_t;
uiCssPropertyList_t* UI_DuplicateCssPropertyList(uiCssPropertyList_t* propertyList);
uiCssPropertyList_t* UI_NewCssPropertyList();
void UI_FreeCssPropertyList(uiCssPropertyList_t *propertyList);

typedef struct uiCssSelectorSection_s {
    enum cssSelectorSectionType {
        UICSS_SELECTOR_SECTION_TYPE_UNKNOWN,
        UICSS_SELECTOR_SECTION_TYPE_TAG,
        UICSS_SELECTOR_SECTION_TYPE_CLASS,
        UICSS_SELECTOR_SECTION_TYPE_ID,
    } type;
    sds value;
    enum cssSelectorSectionAttributeType {
        UICSS_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE,
        UICSS_SELECTOR_SECTION_ATTRIBUTE_TYPE_CLASS,
    } attributeType;
    sds attribute;
} uiCssSelectorSection_t;
uiCssSelectorSection_t* UI_NewCssSelectorSection();
void UI_FreeCssSelectorSection(void *_section);

typedef struct uiCssSelector_s {
    list *sections;
} uiCssSelector_t;
uiCssSelector_t* UI_NewCssSelector();
void UI_FreeCssSelector(uiCssSelector_t *selector);

typedef struct uiCssRule_s {
    uiCssSelector_t *selector;
    uiCssPropertyList_t *propertyList;
} uiCssRule_t;
uiCssRule_t* UI_NewCssRule();
void UI_FreeCssRule(void *_rule);

typedef struct uiCssStyleSheet_s {
    list * rules;
} uiCssStyleSheet_t;
uiCssStyleSheet_t* UI_NewCssStyleSheet();
uiDocumentScanToken_t* UI_ScanCssToken(uiDocumentScanner_t *scanner);
void UI_CssStyleSheetMergeRule(uiCssStyleSheet_t *cssStyleSheet, uiCssRule_t *rule);
const char* UI_ParseCssStyleSheet(uiDocument_t *document, char *cssContent);
void UI_PrintCssStyleSheet(uiCssStyleSheet_t *cssStyleSheet);

/**
 * css选择器
 */
list* UI_ScanLeafHtmlDoms(uiHtmlDom_t *dom);
uiHtmlDom_t* UI_GetHtmlDomByCssSelector(uiDocument_t* document, uiCssSelector_t *selector);

/**
 * render 相关
 */
typedef struct uiRenderObject_s {
    sds   content;
    int   width;
    int   height;
    list  *children;
} uiRenderObject_t;


void UI_PrepareDocument();
typedef struct uiDocument_s {
    char              *content;
    uiHtmlDom_t       *rootDom;
    uiCssStyleSheet_t *cssStyleSheet;
    sds               script;
    sds               style;
    list              *leafHtmlDoms;
} uiDocument_t;
uiDocument_t* UI_NewDocument();
uiDocument_t* UI_ParseDocument(char *documentContent);

#endif
