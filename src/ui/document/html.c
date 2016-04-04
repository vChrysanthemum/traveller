#include <string.h>

#include "core/zmalloc.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/util.h"
#include "core/extern.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

#include "core/extern.h"
#include "ui/extern.h"
#include "ui/document/extern.h"

uiHtmlDomInfo_t *ui_htmlDomInfoUndefined;

static dict *UIHtmlSpecialStringTable;

static dict *uiHtmlDomInfoDict;
static uiHtmlDomInfo_t uiHtmlDomInfoTable[] = {
    {"undefined",UIHTML_DOM_TYPE_UNDEFINED,0,0,0},
    {"head",UIHTML_DOM_TYPE_HEAD,0,0,0},
    {"title",UIHTML_DOM_TYPE_TITLE,0,0,UI_RenderHtmlDomTitle},
    {"script",UIHTML_DOM_TYPE_SCRIPT,0,0,0},
    {"style",UIHTML_DOM_TYPE_STYLE,0,0,0},

    {
        "html",
        UIHTML_DOM_TYPE_HTML,
        HTML_CSS_STYLE_DISPLAY_BLOCK,
        HTML_CSS_STYLE_POSITION_STATIC,
        0
    },

    {
        "text",
        UIHTML_DOM_TYPE_TEXT,
        HTML_CSS_STYLE_DISPLAY_BLOCK,
        HTML_CSS_STYLE_POSITION_STATIC,
        UI_RenderHtmlDomText
    },

    {
        "body",
        UIHTML_DOM_TYPE_BODY,
        HTML_CSS_STYLE_DISPLAY_BLOCK,
        HTML_CSS_STYLE_POSITION_STATIC,
        UI_RenderHtmlDomBody
    },

    {
        "div",
        UIHTML_DOM_TYPE_DIV,
        HTML_CSS_STYLE_DISPLAY_BLOCK,
        HTML_CSS_STYLE_POSITION_RELATIVE,
        UI_RenderHtmlDomDiv
    },

    {
        "table",
        UIHTML_DOM_TYPE_TABLE,
        HTML_CSS_STYLE_DISPLAY_BLOCK,
        HTML_CSS_STYLE_POSITION_RELATIVE,
        UI_RenderHtmlDomTable
    },

    {
        "tr",
        UIHTML_DOM_TYPE_TR,
        HTML_CSS_STYLE_DISPLAY_BLOCK,
        HTML_CSS_STYLE_POSITION_STATIC,
        UI_RenderHtmlDomTr
    },

    {
        "td",
        UIHTML_DOM_TYPE_TD,
        HTML_CSS_STYLE_DISPLAY_INLINE_BLOCK,
        HTML_CSS_STYLE_POSITION_STATIC,
        UI_RenderHtmlDomTd
    },

    {
        "input",
        UIHTML_DOM_TYPE_INPUT,
        HTML_CSS_STYLE_DISPLAY_INLINE_BLOCK,
        HTML_CSS_STYLE_POSITION_RELATIVE,
        UI_RenderHtmlDomInput
    },
    {0,0,0,0,0},
};

static inline void skipStringNotConcern(char **ptr)  {
    while (UI_IsWhiteSpace(**ptr) && 0 != **ptr) {
        (*ptr)++;
    }
}

int UI_IsHtmlDomNotCareCssDeclaration(uiHtmlDom_t *dom) {
    if (UIHTML_DOM_TYPE_TEXT == dom->Info->Type ||
            UIHTML_DOM_TYPE_SCRIPT == dom->Info->Type ||
            UIHTML_DOM_TYPE_STYLE == dom->Info->Type) {
        return 1;
    } else {
        return 0;
    }
}

void UI_PrepareHtml() {
    uiHtmlDomInfoDict = dictCreate(&stackStringTableDictType, 0);
    for (uiHtmlDomInfo_t *domInfo = &uiHtmlDomInfoTable[0]; 0 != domInfo->Name; domInfo++) {
        dictAdd(uiHtmlDomInfoDict, domInfo->Name, domInfo);
    }
    ui_htmlDomInfoUndefined = dictFetchValue(uiHtmlDomInfoDict, "undefined");

    UIHtmlSpecialStringTable = dictCreate(&stackStringTableDictType, 0);
    dictAdd(UIHtmlSpecialStringTable, "&quot;", "\"");
    dictAdd(UIHtmlSpecialStringTable, "&amp;",  "&");
    dictAdd(UIHtmlSpecialStringTable, "&gt;",   ">");
    dictAdd(UIHtmlSpecialStringTable, "&lt;",   "<");
    dictAdd(UIHtmlSpecialStringTable, "&nbsp;", " ");
}

uiDocumentScanToken_t* UI_ScanHtmlToken(uiDocumentScanner_t *scanner) {
    char **ptr = &scanner->Current;

    skipStringNotConcern(ptr);

    if ('\0' == **ptr) {
        return 0;
    }

    int tokenStack[6] = {0};
    int tokenStackLen = 0;

    char *s = *ptr;
    int len;

    uiDocumentScanToken_t *token = UI_NewDocumentScanToken();
    token->Type = UIHTML_TOKEN_TEXT;

    while ('\0' != *s && tokenStackLen < 6) {
        // 解析覆盖一下情况
        // "<text>"
        // "<text/>"
        // "</text>"
        // "text"
        // "<text<text>"

        switch (*s) {
            case '<':
                //预防 "< lkasfd < >" 的情况
                if (0 == tokenStackLen) {
                    tokenStack[0] = '<';
                    tokenStackLen++;
                } else {
                    if ('<' == tokenStack[0]) {
                        if (UIHTML_TOKEN_TEXT != tokenStack[tokenStackLen-1]) {
                            tokenStack[tokenStackLen] = UIHTML_TOKEN_TEXT;
                            tokenStackLen++;
                        }
                    } else {
                        tokenStack[tokenStackLen] = '<';
                        tokenStackLen++;
                    }
                }
                break;
            case '>':
                tokenStack[tokenStackLen] = '>';
                tokenStackLen++;
                break;
            case '/':
                tokenStack[tokenStackLen] = '/';
                tokenStackLen++;
                break;
            default:
                if (0 == tokenStackLen) {
                    tokenStack[0] = UIHTML_TOKEN_TEXT;
                    tokenStackLen++;
                } else if (UIHTML_TOKEN_TEXT != tokenStack[tokenStackLen-1]) {
                    tokenStack[tokenStackLen] = UIHTML_TOKEN_TEXT;
                    tokenStackLen++;
                }
        }

        if (3 == tokenStackLen &&
                '<' == tokenStack[0]  &&   // <
                UIHTML_TOKEN_TEXT == tokenStack[1] &&   // tag
                '>' == tokenStack[2]) {    // >
            token->Type = UIHTML_TOKEN_START_TAG;
            goto GET_TOKEN_SUCCESS;

        } else if (4 == tokenStackLen) {
            if ('<' == tokenStack[0]      && // <
                    '/' == tokenStack[1]  && // /
                    UIHTML_TOKEN_TEXT == tokenStack[2] && // tag
                    '>' == tokenStack[3]) {    // >
                token->Type = UIHTML_TOKEN_END_TAG;
                goto GET_TOKEN_SUCCESS;

            } else if ('<' == tokenStack[0] && // <
                    UIHTML_TOKEN_TEXT == tokenStack[1]   && // tag
                    '/' == tokenStack[2]    && // /
                    '>' == tokenStack[3]) {    // >
                token->Type = UIHTML_TOKEN_SELF_CLOSING_TAG;
                goto GET_TOKEN_SUCCESS;
            }

        } else if (2 == tokenStackLen &&
                UIHTML_TOKEN_TEXT == tokenStack[0] &&    // text
                '<' == tokenStack[1]) {     // <
            token->Type = UIHTML_TOKEN_TEXT;
            tokenStackLen--;
            s--;
            goto GET_TOKEN_SUCCESS;
        }

        s++;
    }

GET_TOKEN_SUCCESS:

    len = (int)(s-*ptr+1);

    //去掉尾部空格
    if (UIHTML_TOKEN_TEXT == token->Type) {
        s = &((*ptr)[len-1]);
        for (int i = len-1; i >= 0; i--,s--) {
            if (UI_IsWhiteSpace(*s)) {
                continue;
            }
            break;
        }
    }

    token->Content = sdscatlen(token->Content, *ptr, (int)(s-*ptr+1));
    (*ptr) = (*ptr) + len;
    return token;
}

static void InitialHtmlDomStyleByHtmlDomInfo(uiHtmlDom_t *dom) {
    dom->Style.Display = dom->Info->InitialStyleDisplay;
    dom->Style.Position = dom->Info->InitialStylePosition;

    if (0 != dom->Parent) {
        dom->Style.TextAlign = dom->Parent->Style.TextAlign;
    } else {
        dom->Style.TextAlign = LEFT;
    }
}

uiHtmlDom_t* UI_NewHtmlDom(uiHtmlDom_t *parentDom) {
    uiHtmlDom_t *dom = (uiHtmlDom_t*)zmalloc(sizeof(uiHtmlDom_t));
    memset(dom, 0, sizeof(uiHtmlDom_t));
    dom->Parent = parentDom;
    dom->Title = sdsempty();
    dom->Children = listCreate();
    dom->Children->free = UI_FreeHtmlDom;
    dom->Info = ui_htmlDomInfoUndefined;
    dom->CssDeclarations = listCreate();
    dom->CssDeclarations->free = UI_FreeCssDeclaration;

    dom->Style.TextAlign = LEFT;
    dom->Style.Display = HTML_CSS_STYLE_DISPLAY_INLINE_BLOCK;

    memset(&(dom->Style), 0, sizeof(uiHtmlDomStyle_t));
    return dom;
}

void UI_FreeHtmlDom(void *_dom) {
    uiHtmlDom_t *dom = (uiHtmlDom_t*)_dom;
    sdsfree(dom->Title);
    if (0 != dom->Attributes) listRelease(dom->Attributes);
    if (0 != dom->Id) sdsfree(dom->Id);
    if (0 != dom->Classes) listRelease(dom->Classes);
    if (0 != dom->StyleCssDeclarations) listRelease(dom->StyleCssDeclarations);
    listRelease(dom->CssDeclarations);
    if (0 != dom->Content) sdsfree(dom->Content);
    listRelease(dom->Children);
}

// 解析单个tag 中的 title和attribute
static inline void parseHtmlDomTag(uiHtmlDom_t *dom, uiDocumentScanToken_t *token) {
    int contentLen = sdslen(token->Content);

    uiHtmlDomInfo_t *htmlDomInfo;
    int contentOffset;
    int contentEndOffset;
    enum {EXPECTING_KEY, EXPECTING_VALUE} expectState;

    // 提取 title
    contentOffset = 0;
    for (contentEndOffset = 1; contentEndOffset < contentLen; contentEndOffset++) {
        if (UI_IsWhiteSpace(token->Content[contentEndOffset])) {
            break;
        }
    }
    dom->Title = sdscatlen(dom->Title, &(token->Content[contentOffset]), contentEndOffset-contentOffset);
    htmlDomInfo = dictFetchValue(uiHtmlDomInfoDict, dom->Title);
    if (0 != htmlDomInfo) {
        dom->Info = htmlDomInfo;
        InitialHtmlDomStyleByHtmlDomInfo(dom);
    }

    contentOffset = contentEndOffset + 1;

    int len = strlen(&token->Content[contentOffset]);
    if (0 == len) {
        return;
    }

    doubleString_t *domAttribute;
    sds attributeData = sdsnewlen(&token->Content[contentOffset], len);
    int attributeDataOffset;
    char *attributeDataPtr = &token->Content[contentOffset];
    char *attributeKey = 0;
    char *attributeValue = 0;
    char *sptr, *sptr2;

    // 提取attribute
    expectState = EXPECTING_KEY;
    while (1) {
        // 开始解析新的 key 或 value

        // 跳过空格
        if (UI_IsWhiteSpace(*attributeDataPtr)) {
            attributeDataPtr++;
        }

        if ('\'' == *attributeDataPtr || '"' == *attributeDataPtr) {
            escapeQuoteContent(attributeData, &attributeDataPtr);

        } else {
            attributeDataOffset = -1;
            attributeDataPtr--;
            while (1) {
                attributeDataOffset++;
                attributeDataPtr++;

                if ('\0' == *attributeDataPtr) {
                    break;
                }

                if (UI_IsWhiteSpace(*attributeDataPtr)) {
                    attributeDataPtr++;
                    break;
                }

                if ('=' == *attributeDataPtr) {
                    attributeDataPtr++;
                    break;
                }

                if ('\\' == *attributeDataPtr) {
                    attributeDataPtr++;
                    attributeData[attributeDataOffset] = *attributeDataPtr;
                    continue;
                }

                attributeData[attributeDataOffset] = *attributeDataPtr;
            }
            attributeData[attributeDataOffset] = '\0';
        }

        switch (expectState) {
            case EXPECTING_KEY:
                attributeKey = stringnewlen(attributeData, strlen(attributeData));
                expectState = EXPECTING_VALUE;
                break;
            case EXPECTING_VALUE:
                attributeValue = stringnewlen(attributeData, strlen(attributeData));
                expectState = EXPECTING_KEY;

                if (0 == dom->Attributes) {
                    dom->Attributes = listCreate();
                    dom->Attributes->free = freeDoubleString;
                }

                domAttribute = newDoubleString();
                domAttribute->v1 = attributeKey;
                domAttribute->v2 = attributeValue;
                dom->Attributes = listAddNodeTail(dom->Attributes, domAttribute);

                if (0 == stringcmp("class", attributeKey)) {
                    if (0 == dom->Classes) {
                        dom->Classes = listCreate();
                        dom->Classes->free = listFreeSds;
                    }

                    sptr = attributeValue;
                    while (UI_IsWhiteSpace(*sptr)) {
                        sptr++;
                    }
                    sptr2 = sptr + 1;
                    while (1) {
                        if ('\0' == *sptr2) {
                            break;
                        }

                        if (UI_IsWhiteSpace(*sptr2)) {
                            listAddNodeTail(dom->Classes, sdsnewlen(sptr, sptr2-sptr));

                            while (UI_IsWhiteSpace(*sptr2)) {
                                sptr2++;
                            }

                            sptr = sptr2;

                            if ('\0' == *sptr2) {
                                break;
                            }
                        }

                        sptr2++;
                    }

                    if (sptr2 > sptr) {
                        listAddNodeTail(dom->Classes, sdsnewlen(sptr, sptr2-sptr));
                    }

                } else if (0 == stringcmp("id", attributeKey)) {
                    if (0 != dom->Id) {
                        sdsfree(dom->Id);
                    }

                    dom->Id = sdsnew(attributeValue);

                } else if (0 == stringcmp("style", attributeKey)) {
                    UI_CompileCssDeclarations(&dom->StyleCssDeclarations, attributeValue);
                }

                attributeKey = 0;
                attributeValue = 0;
                break;
        }

        if ('\0' == *attributeDataPtr) {
            goto PARSE_HTML_DOM_TAG_END;
        }
    }

PARSE_HTML_DOM_TAG_END:
    if (0 != attributeData) sdsfree(attributeData);
    if (0 != attributeKey) zfree(attributeKey);
    if (0 != attributeValue) zfree(attributeValue);
}

// @return uiHtmlDom_t* 需要继续处理的 uiHtmlDom_t*

// <tag>
static inline uiHtmlDom_t* parseHtmlTokenStartTag(uiHtmlDom_t *dom, uiDocumentScanToken_t *token) {
    sdsrange(token->Content, 1, -2);
    uiHtmlDom_t *newdom = UI_NewHtmlDom(dom);
    parseHtmlDomTag(newdom, token);
    dom->Children = listAddNodeTail(dom->Children, newdom);
    return newdom;
}

// </tag>
static inline uiHtmlDom_t* parseHtmlTokenEndTag(uiHtmlDom_t *dom, uiDocumentScanToken_t *token) {
    sdsrange(token->Content, 2, -2);
    return dom->Parent;
}

// <tag/>
static inline uiHtmlDom_t* parseHtmlTokenSelfClosingTag(uiHtmlDom_t *dom, uiDocumentScanToken_t *token) {
    sdsrange(token->Content, 1, -3);
    uiHtmlDom_t *newdom = UI_NewHtmlDom(dom);
    parseHtmlDomTag(newdom, token);
    dom->Children = listAddNodeTail(dom->Children, newdom);
    return dom;
}

// text
static inline uiHtmlDom_t* parseHtmlTokenText(uiDocument_t *document,
        uiHtmlDom_t *dom, uiDocumentScanToken_t *token) {

    if (UIHTML_DOM_TYPE_SCRIPT == dom->Info->Type) {
        document->Script = sdscatsds(document->Script, token->Content);

    } else if (UIHTML_DOM_TYPE_STYLE == dom->Info->Type) {
        document->Style = sdscatsds(document->Style, token->Content);

    } else {
        // 除了 script 以外的dom内容，均作特殊处理
        uiHtmlDom_t *newdom = UI_NewHtmlDom(dom);
        newdom->Content = sdsempty();
        newdom->Info = dictFetchValue(uiHtmlDomInfoDict, "text");
        InitialHtmlDomStyleByHtmlDomInfo(newdom);
        dom->Children = listAddNodeTail(dom->Children, newdom);

        newdom->Content = sdsMakeRoomFor(newdom->Content,
                sdslen(newdom->Content)+sdslen(token->Content));

        int domPoi = sdslen(newdom->Content);
        int isWhiteSpaceFound = 0;
        int len = sdslen(token->Content);
        char *ptr = token->Content;
        char *ptr2;
        sds tmpString = sdsMakeRoomFor(sdsempty(), 12);
        for (int i = 0; i < len; i++,ptr++) {
            //合并空格
            if (UI_IsWhiteSpace(*ptr)) {
                if (0 == isWhiteSpaceFound) {
                    isWhiteSpaceFound = 1;
                    newdom->Content[domPoi] = ' ';
                    domPoi++;
                }
                continue;

            }

            if (1 == isWhiteSpaceFound) {
                isWhiteSpaceFound = 0;
            }

            //转义
            if ('&' == *ptr) {
                ptr2 = ptr;
                while (i < len && ';' != *ptr2) {
                    i++;ptr2++;
                }
                sdsclear(tmpString);
                tmpString = sdscatlen(tmpString, ptr, ptr2-ptr+1);
                ptr = ptr2;
                ptr2 = dictFetchValue(UIHtmlSpecialStringTable, tmpString);
                if (0 != ptr2) {
                    newdom->Content[domPoi] = *ptr2;
                    domPoi++;
                }
                continue;
            }

            newdom->Content[domPoi] = *ptr;
            domPoi++;
        }
        sdsfree(tmpString);
        newdom->Content[domPoi] = '\0';
        newdom->ContentUtf8Width = utf8StrWidth(newdom->Content);
        sdsupdatelen(newdom->Content);
    }

    return dom;
}

int UI_ParseHtml(uiDocument_t *document) {
    document->RootDom = UI_NewHtmlDom(0);
    uiHtmlDom_t *dom = document->RootDom;
    uiDocumentScanner_t htmlScanner = {
        0, document->Content, document->Content, UI_ScanHtmlToken
    };

    uiDocumentScanToken_t *token;
    while (0 != (token = htmlScanner.Scan(&htmlScanner))) {
        switch (token->Type) {
            case UIHTML_TOKEN_START_TAG:
                dom = parseHtmlTokenStartTag(dom, token);
                break;
            case UIHTML_TOKEN_END_TAG:
                dom = parseHtmlTokenEndTag(dom, token);
                break;
            case UIHTML_TOKEN_SELF_CLOSING_TAG:
                dom = parseHtmlTokenSelfClosingTag(dom, token);
                break;
            case UIHTML_TOKEN_TEXT:
                dom = parseHtmlTokenText(document, dom, token);
                break;
        }

        UI_FreeDocumentScanToken(token);
    }

    return 0;
}

void UI_PrintHtmlDomTree(uiHtmlDom_t *dom, int indent) {
    int i;
    for (i = 0; i < indent; i++) { printf("  "); }
    if (0 == UI_IsHtmlDomNotCareCssDeclaration(dom)) {
        printf("<%s:%s>", dom->Title, dom->Info->Name);
    }
    if (0 != dom->Attributes && listLength(dom->Attributes) > 0) {
        listIter *liAttribute;
        listNode *lnAttribute;
        doubleString_t *attribute;
        liAttribute = listGetIterator(dom->Attributes, AL_START_HEAD);
        while (0 != (lnAttribute = listNext(liAttribute))) {
            attribute = (doubleString_t*)listNodeValue(lnAttribute);
            printf(" |%s=%s| ", attribute->v1, attribute->v2);
        }
        listReleaseIterator(liAttribute);
    }

    if (1 == UI_IsHtmlDomNotCareCssDeclaration(dom)) {
        printf("(%s) %s\n", dom->Info->Name, dom->Content);

    } else {
        printf("\n");
    }

    listIter *li;
    listNode *ln;
    li = listGetIterator(dom->Children, AL_START_HEAD);
    indent++;
    while (0 != (ln = listNext(li))) {
        UI_PrintHtmlDomTree((uiHtmlDom_t*)listNodeValue(ln), indent);
    }
    listReleaseIterator(li);
}
