#ifndef __UI_DOCUMENT_H
#define __UI_DOCUMENT_H

typedef struct uiDocumentScanToken_s uiDocumentScanToken_t;
typedef struct uiDocumentScanner_s uiDocumentScanner_t;
typedef struct uiDocument_s uiDocument_t;
typedef struct uiDocumentRenderObject_s uiDocumentRenderObject_t;

uiDocumentScanToken_t* UI_ScanHtmlToken(uiDocumentScanner_t *scanner);

/**
 * css 相关
 */
#define UI_PARSE_STATE_SELECTOR         2 // 解析selector阶段
#define UI_PARSE_STATE_CSS_DECLARATION_KEY     5 // 解析cssDeclaration中的key阶段
#define UI_PARSE_STATE_CSS_DECLARATION_VALUE   6 // 解析cssDeclaration中的value阶段

#define UI_TOKEN_TEXT            '*' // 字符串
#define UI_TOKEN_COMMA           ',' // ,
#define UI_TOKEN_COLON           ':' // :
#define UI_TOKEN_SEMICOLON       ';' // ;
#define UI_TOKEN_BLOCK_START     '{' // {
#define UI_TOKEN_BLOCK_END       '}' // }

void UI_PrepareCss();

typedef struct uiCssDeclarationInfo_s {
    char *name;
    enum uiCssDeclarationType_e {
        UI_CSS_DECLARATION_TYPE_UNKNOWN,
        UI_CSS_DECLARATION_TYPE_BACKGROUND_COLOR,
        UI_CSS_DECLARATION_TYPE_COLOR,
        UI_CSS_DECLARATION_TYPE_PADDING,
        UI_CSS_DECLARATION_TYPE_PADDING_TOP,
        UI_CSS_DECLARATION_TYPE_PADDING_BOTTOM,
        UI_CSS_DECLARATION_TYPE_PADDING_LEFT,
        UI_CSS_DECLARATION_TYPE_PADDING_RIGHT,
        UI_CSS_DECLARATION_TYPE_MARGIN,
        UI_CSS_DECLARATION_TYPE_MARGIN_TOP,
        UI_CSS_DECLARATION_TYPE_MARGIN_BOTTOM,
        UI_CSS_DECLARATION_TYPE_MARGIN_LEFT,
        UI_CSS_DECLARATION_TYPE_MARGIN_RIGHT,
        UI_CSS_DECLARATION_TYPE_DISPLAY,
        UI_CSS_DECLARATION_TYPE_TEXT_ALIGN,
        UI_CSS_DECLARATION_TYPE_WIDTH,
        UI_CSS_DECLARATION_TYPE_HEIGHT,
    } type;
} uiCssDeclarationInfo_t;

typedef struct uiCssDeclaration_s {
    enum uiCssDeclarationType_e type;
    sds key;
    sds value;
} uiCssDeclaration_t;
uiCssDeclaration_t* UI_NewCssDeclaration();
void UI_FreeCssDeclaration(void *_cssDeclaration);

typedef struct uiCssDeclarationList_s {
    int referenceCount;
    list *data;
} uiCssDeclarationList_t;
uiCssDeclarationList_t* UI_DuplicateCssDeclarationList(uiCssDeclarationList_t* cssDeclarationList);
uiCssDeclarationList_t* UI_NewCssDeclarationList();
void UI_FreeCssDeclarationList(uiCssDeclarationList_t *cssDeclarationList);

typedef struct uiCssSelectorSection_s {
    enum cssSelectorSectionType {
        UI_SELECTOR_SECTION_TYPE_UNKNOWN,
        UI_SELECTOR_SECTION_TYPE_TAG,
        UI_SELECTOR_SECTION_TYPE_CLASS,
        UI_SELECTOR_SECTION_TYPE_ID,
    } type;
    sds value;
    enum cssSelectorSectionAttributeType {
        UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE,
        UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_CLASS,
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
    uiCssDeclarationList_t *cssDeclarationList;
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

void UI_ParseSdsToCssDeclaration(uiCssDeclaration_t *cssDeclaration, sds cssDeclarationKey, sds cssDeclarationValue);
void UI_CompileCssDeclarations(list **cssDeclarations, char *code);
void UI_CompileCssSelector(uiCssSelector_t **selector, char *code);

/**
 * html 相关
 */
#define UIHTML_TOKEN_TEXT               -1 // 标记中的字符串
#define UIHTML_TOKEN_START_TAG          -2 // <tag>
#define UIHTML_TOKEN_END_TAG            -3 // </tag>
#define UIHTML_TOKEN_SELF_CLOSING_TAG   -4 // <tag />

void UI_PrepareHtml();

typedef struct uiHtmlDom_s uiHtmlDom_t;
typedef void (*UI_ComputeHtmlDomStyle)(uiDocument_t*, uiHtmlDom_t*);
void UI_ComputeHtmlDomStyle_Text      (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Html      (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Head      (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Title     (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Body      (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Div       (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Table     (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Tr        (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Td        (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_ComputeHtmlDomStyle_Input     (uiDocument_t *document, uiHtmlDom_t *dom);

typedef struct uiHtmlDomInfo_s {
    char *name;
    enum uiHtmlDomType_e {
        UIHTML_DOM_TYPE_UNDEFINED,
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
    UI_ComputeHtmlDomStyle computeStyle;
} uiHtmlDomInfo_t;

typedef struct uiHtmlDom_s {
    uiHtmlDom_t               *parent;
    sds                       title;
    list                      *attributes;// list doubleString_t
    sds                       id;
    list                      *classes;  // list sds
    list                      *styleCssDeclarations;
    sds                       content;
    int                       contentUtf8Width;
    list                      *children;
    uiDocumentRenderObject_t  *renderObject;
    uiHtmlDomInfo_t           *info;
} uiHtmlDom_t;
int UI_IsHtmlDomNotCareCssDeclaration(uiHtmlDom_t *dom);
uiHtmlDom_t* UI_NewHtmlDom(uiHtmlDom_t *parentDom);
void UI_FreeHtmlDom(void *_dom);
int UI_ParseHtml(uiDocument_t *document);

void UI_PrintHtmlDomTree(uiHtmlDom_t *dom, int indent);

/**
 * css选择器
 */
list* UI_ScanLeafHtmlDoms(uiHtmlDom_t *dom);
list* UI_GetHtmlDomsByCssSelector(uiDocument_t* document, uiCssSelector_t *selector);

/**
 * render 相关
 */
typedef struct uiDocumentRenderObject_s {
    unsigned int paddingTop;
    unsigned int paddingBottom;
    unsigned int paddingLeft;
    unsigned int paddingRight;
    unsigned int marginTop;
    unsigned int marginBottom;
    unsigned int marginLeft;
    unsigned int marginRight;
    unsigned int backgroundColor;
    unsigned int color;
    int          width;
    int          height;
    int          positionStartX;
    int          positionStartY;
    unsigned int isHide;
    sds          textAlign;
    uiHtmlDom_t  *dom;
} uiDocumentRenderObject_t;
uiDocumentRenderObject_t *UI_newDocumentRenderObject(uiHtmlDom_t *dom);
void UI_freeDocumentRenderObject(uiDocumentRenderObject_t *renderObject);

void UI_ComputeHtmlDomTreeStyle(uiDocument_t *document);

/**
 * document 相关
 */
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

void UI_PrepareDocument();
typedef struct uiDocument_s {
    char              *content;
    uiHtmlDom_t       *rootDom;
    uiCssStyleSheet_t *cssStyleSheet;
    sds               script;
    sds               style;
} uiDocument_t;
uiDocument_t* UI_NewDocument();
uiDocument_t* UI_ParseDocument(char *documentContent);

#endif
