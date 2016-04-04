#ifndef __UI_DOCUMENT_H
#define __UI_DOCUMENT_H

typedef struct uiDocumentScanToken_s uiDocumentScanToken_t;
typedef struct uiDocumentScanner_s uiDocumentScanner_t;
typedef struct uiDocument_s uiDocument_t;

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
    char *Name;
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
        UI_CSS_DECLARATION_TYPE_POSITION,
        UI_CSS_DECLARATION_TYPE_LEFT,
        UI_CSS_DECLARATION_TYPE_RIGHT,
        UI_CSS_DECLARATION_TYPE_TOP,
        UI_CSS_DECLARATION_TYPE_BOTTOM,
    } Type;
} uiCssDeclarationInfo_t;

typedef struct uiCssDeclaration_s {
    enum uiCssDeclarationType_e Type;
    sds Key;
    sds Value;
} uiCssDeclaration_t;
uiCssDeclaration_t* UI_NewCssDeclaration();
void UI_FreeCssDeclaration(void *_cssDeclaration);

typedef struct uiCssDeclarationList_s {
    int referenceCount;
    list *Data;
} uiCssDeclarationList_t;
void UI_UpdateCssDeclaration(uiCssDeclaration_t *src, uiCssDeclaration_t *dst);
uiCssDeclaration_t* UI_DuplicateCssDeclaration(uiCssDeclaration_t *dst);
uiCssDeclarationList_t* UI_DuplicateCssDeclarationList(uiCssDeclarationList_t* cssDeclarationList);
uiCssDeclarationList_t* UI_NewCssDeclarationList();
void UI_FreeCssDeclarationList(uiCssDeclarationList_t *cssDeclarationList);

typedef struct uiCssSelectorSection_s {
    enum cssSelectorSectionType {
        UI_SELECTOR_SECTION_TYPE_UNKNOWN,
        UI_SELECTOR_SECTION_TYPE_TAG,
        UI_SELECTOR_SECTION_TYPE_CLASS,
        UI_SELECTOR_SECTION_TYPE_ID,
    } Type;
    sds Value;
    enum cssSelectorSectionAttributeType {
        UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE,
        UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_CLASS,
    } AttributeType;
    sds Attribute;
} uiCssSelectorSection_t;
uiCssSelectorSection_t* UI_NewCssSelectorSection();
void UI_FreeCssSelectorSection(void *_section);

typedef struct uiCssSelector_s {
    list *Sections;
} uiCssSelector_t;
uiCssSelector_t* UI_NewCssSelector();
void UI_FreeCssSelector(uiCssSelector_t *selector);

typedef struct uiCssRule_s {
    uiCssSelector_t         *Selector;
    uiCssDeclarationList_t  *CssDeclarationList;
} uiCssRule_t;
uiCssRule_t* UI_NewCssRule();
void UI_FreeCssRule(void *_rule);

typedef struct uiCssStyleSheet_s {
    list * Rules;
} uiCssStyleSheet_t;
uiCssStyleSheet_t* UI_NewCssStyleSheet();
void UI_FreeCssStyleSheet(uiCssStyleSheet_t *cssStyleSheet);
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
typedef void (*UI_RenderHtmlDom)(uiDocument_t*, uiHtmlDom_t*);
void UI_RenderHtmlDomText      (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_RenderHtmlDomTitle     (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_RenderHtmlDomBody      (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_RenderHtmlDomDiv       (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_RenderHtmlDomTable     (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_RenderHtmlDomTr        (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_RenderHtmlDomTd        (uiDocument_t *document, uiHtmlDom_t *dom);
void UI_RenderHtmlDomInput     (uiDocument_t *document, uiHtmlDom_t *dom);

typedef struct uiHtmlDomStyle_s {
    unsigned int PaddingTop;
    unsigned int PaddingBottom;
    unsigned int PaddingLeft;
    unsigned int PaddingRight;
    unsigned int MarginTop;
    unsigned int MarginBottom;
    unsigned int MarginLeft;
    unsigned int MarginRight;
    unsigned int BackgroundColor;
    unsigned int Color;
    int          IsWidthPercent;
    int          WidthPercent;
    int          Width;
    int          Height;
    int          PositionStartX;
    int          PositionStartY;
    int          TextAlign;
    enum uiHtmlCssStyleDisplay_e {
        HTML_CSS_STYLE_DISPLAY_NONE,
        HTML_CSS_STYLE_DISPLAY_BLOCK,
        HTML_CSS_STYLE_DISPLAY_INLINE_BLOCK,
    } Display;
    enum uiHtmlCssStylePosition_e {
        HTML_CSS_STYLE_POSITION_RELATIVE,
        HTML_CSS_STYLE_POSITION_ABSOLUTE,
        HTML_CSS_STYLE_POSITION_FIXED,
        HTML_CSS_STYLE_POSITION_STATIC,
    } Position;
    int          Left;
    int          Right;
    int          Top;
    int          Bottom;
} uiHtmlDomStyle_t;

typedef struct uiHtmlDomInfo_s {
    char *Name;
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
    } Type;
    enum uiHtmlCssStyleDisplay_e  InitialStyleDisplay;
    enum uiHtmlCssStylePosition_e InitialStylePosition;
    UI_RenderHtmlDom Render;
} uiHtmlDomInfo_t;

typedef struct uiHtmlDom_s {
    int                       layoutChildrenWidth;
    int                       layoutCurrentRowHeight;
    int                       layoutChildrenHeight;
    uiHtmlDom_t               *Parent;
    sds                       Title;
    list                      *Attributes;// list doubleString_t
    sds                       Id;
    list                      *Classes;  // list sds
    list                      *StyleCssDeclarations;
    list                      *CssDeclarations;
    sds                       Content;
    int                       ContentUtf8Width;
    list                      *Children;
    uiHtmlDomStyle_t          Style;
    uiHtmlDomInfo_t           *Info;
} uiHtmlDom_t;
int UI_IsHtmlDomNotCareCssDeclaration(uiHtmlDom_t *dom);
uiHtmlDom_t* UI_NewHtmlDom(uiHtmlDom_t *parentDom);
void UI_FreeHtmlDom(void *_dom);
int UI_ParseHtml(uiDocument_t *document);

void UI_PrintHtmlDomTree(uiHtmlDom_t *dom, int indent);

/**
 * css选择器
 */
int UI_IsSelectorSectionsLeftMatchHtmlDoms(listIter *liSelectorSection, uiHtmlDom_t *dom);
list* UI_ScanLeafHtmlDoms(uiHtmlDom_t *dom);
list* UI_GetHtmlDomsByCssSelector(uiDocument_t* document, uiCssSelector_t *selector);

/**
 * render 相关
 */
typedef struct uiLayoutDocumentEnvironment_s {
    int StartX;
    int StartY;
    int Width;
} uiLayoutDocumentEnvironment_t;
void UI_ComputeHtmlDomTreeStyle(uiDocument_t *document);
void UI_LayoutDocument(uiDocument_t *document, int winWidth);
void UI_RenderHtmlDomTree(uiDocument_t *document, uiHtmlDom_t *dom);

/**
 * document 相关
 */
typedef struct uiDocumentScanToken_s {
    int  Type;
    sds  Content;
} uiDocumentScanToken_t;
uiDocumentScanToken_t* UI_NewDocumentScanToken();
void UI_FreeDocumentScanToken(uiDocumentScanToken_t *token);

typedef struct uiDocumentScanner_s uiDocumentScanner_t;
typedef struct uiDocumentScanner_s {
    int  State;
    char *Content;
    char *Current;
    uiDocumentScanToken_t* (*Scan) (uiDocumentScanner_t *scanner);
} uiDocumentScanner_t;

void UI_PrepareDocument();
typedef struct uiDocument_s {
    char                            *Content;
    uiHtmlDom_t                     *RootDom;
    uiCssStyleSheet_t               *CssStyleSheet;
    sds                             Title;
    sds                             Script;
    sds                             Style;
    uiLayoutDocumentEnvironment_t   layoutEnvironment;
} uiDocument_t;
uiDocument_t* UI_NewDocument();
void UI_FreeDocument(uiDocument_t* document);
uiDocument_t* UI_ParseDocument(char *documentContent);
void UI_RenderDocument(uiDocument_t *document, int winWidth);

#endif
