#include "core/stack.h"
#include "core/sds.h"
#include "ui/ui.h"

#include "core/extern.h"
#include "ui/extern.h"

static dict *UIHtmlSpecialStringTable;
static UIHtmlDomInfo UIHtmlDomInfoTable[] = {
    {"unknown"},
    {"text"},
    {"html"},
    {"head"},
    {"title"},
    {"body"},
    {"script"},
    {"div"},
    {"table"},
    {"tr"},
    {"td"},
    {"style"},
    {0},
};
const int UIHtmlDomInfoTableSize = (sizeof(UIHtmlDomInfoTable)/sizeof(UIHtmlDomInfo));

static dict *UIHtmlDomInfoDict;

void UIPrepareHtml() {
    UIHtmlDomInfoDict = dictCreate(&stackStringTableDictType, 0);
    for (UIHtmlDomInfo *domInfo = &UIHtmlDomInfoTable[0]; 0 != domInfo->name; domInfo++) {
        dictAdd(UIHtmlDomInfoDict, domInfo->name, domInfo);
    }

    UIHtmlSpecialStringTable = dictCreate(&stackStringTableDictType, 0);
    dictAdd(UIHtmlSpecialStringTable, "&quot;", "\"");
    dictAdd(UIHtmlSpecialStringTable, "&amp;",  "&");
    dictAdd(UIHtmlSpecialStringTable, "&gt;",   ">");
    dictAdd(UIHtmlSpecialStringTable, "&lt;",   "<");
    dictAdd(UIHtmlSpecialStringTable, "&nbsp;", " ");
}

static inline void skipStringNotConcern(char **ptr)  {
    while(UIIsWhiteSpace(**ptr) && 0 != **ptr) {
        (*ptr)++;
    }
}

static inline UIHtmlToken* newHtmlToken() {
    UIHtmlToken *token = (UIHtmlToken*)zmalloc(sizeof(UIHtmlToken));
    token->content = sdsempty();
    return token;
}

void UIHtmlFreeToken(UIHtmlToken *token) {
    sdsfree(token->content);
    zfree(token);
}

UIHtmlToken* UIHtmlNextToken(char **ptr) {
    skipStringNotConcern(ptr);

    if ('\0' == **ptr) {
        return 0;
    }

    int tokenStack[6] = {0};
    int tokenStackLen = 0;

    char *s = *ptr;
    int len;
    
    UIHtmlToken *token = newHtmlToken();
    token->type = UIHTML_TOKEN_TEXT;

    while('\0' != *s && tokenStackLen < 6) {
        // 解析覆盖一下情况
        // "<text>"
        // "<text/>"
        // "</text>"
        // "text"
        // "<text<text>"

        switch(*s) {
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
            token->type = UIHTML_TOKEN_START_TAG;
            goto GET_TOKEN_SUCCESS;

        } else if (4 == tokenStackLen) {
            if ('<' == tokenStack[0]      && // <
                    '/' == tokenStack[1]  && // /
                    UIHTML_TOKEN_TEXT == tokenStack[2] && // tag
                    '>' == tokenStack[3]) {    // >
                token->type = UIHTML_TOKEN_END_TAG;
                goto GET_TOKEN_SUCCESS;

            } else if ('<' == tokenStack[0] && // <
                    UIHTML_TOKEN_TEXT == tokenStack[1]   && // tag
                    '/' == tokenStack[2]    && // /
                    '>' == tokenStack[3]) {    // >
                token->type = UIHTML_TOKEN_SELF_CLOSING_TAG;
                goto GET_TOKEN_SUCCESS;
            }

        } else if (2 == tokenStackLen && 
                UIHTML_TOKEN_TEXT == tokenStack[0] &&    // text
                '<' == tokenStack[1]) {     // <
            token->type = UIHTML_TOKEN_TEXT;
            tokenStackLen--;
            s--;
            goto GET_TOKEN_SUCCESS;
        }

        s++;
    }

GET_TOKEN_SUCCESS:

    len = (int)(s-*ptr+1);

    //去掉尾部空格
    if (UIHTML_TOKEN_TEXT == token->type) {
        s = &((*ptr)[len-1]);
        for (int i = len-1; i >= 0; i--,s--) {
            if (UIIsWhiteSpace(*s)) {
                continue;
            }
            break;
        }
    }

    token->content = sdscatlen(token->content, *ptr, (int)(s-*ptr+1));
    (*ptr) = (*ptr) + len;
    return token;
}

UIHtmlDom* UINewHtmlDom() {
    UIHtmlDom *dom = (UIHtmlDom*)zmalloc(sizeof(UIHtmlDom));
    memset(dom, 0, sizeof(UIHtmlDom));
    dom->title = sdsempty();
    dom->type = "unknown";
    dom->children = listCreate();
    dom->children->free = UIFreeHtmlDom;
    return dom;
}

void UIFreeHtmlDom(void *_dom) {
    UIHtmlDom *dom = (UIHtmlDom*)_dom;
    sdsfree(dom->title);
    if (0 != dom->attribute) dictRelease(dom->attribute);
    if (0 != dom->id) sdsfree(dom->id);
    if (0 != dom->classes) listRelease(dom->classes);
    if (0 != dom->content) sdsfree(dom->content);
    listRelease(dom->children);
}

// 解析单个tag 中的 title和attribute
static inline void parseHtmlDomTag(UIHtmlDom *dom, UIHtmlToken *token) {
    int contentLen = sdslen(token->content);

    int contentOffset;
    int contentEndOffset;
    enum {EXPECTING_KEY, EXPECTING_VALUE} expectState;
    int isExpectingDoubleQuoteClose, isExpectingQuoteClose;

    // 提取 title
    contentOffset = 0;
    for (contentEndOffset = 1; contentEndOffset < contentLen; contentEndOffset++) {
        if (UIIsWhiteSpace(token->content[contentEndOffset])) {
            break;
        }
    }
    dom->title = sdscatlen(dom->title, &(token->content[contentOffset]), contentEndOffset-contentOffset);
    UIHtmlDomInfo *domInfo = dictFetchValue(UIHtmlDomInfoDict, dom->title);
    if (0 == domInfo) {
        dom->type = "unknown";
    } else {
        dom->type = domInfo->name;
    }

    contentOffset = contentEndOffset + 1;

    int len = strlen(&token->content[contentOffset]);
    if (0 == len) {
        return;
    }

    sds attributeData = sdsnewlen(&token->content[contentOffset], len);
    int attributeDataOffset;
    char *attributeDataPtr = &token->content[contentOffset];
    char *attributeKey = 0;
    char *attributeValue = 0;
    char *sptr, *sptr2;
    attributeDataOffset = 0;

    // 提取attribute
    expectState = EXPECTING_KEY;
    while(1) {
        // 开始解析新的 key 或 value

        // 跳过空格
        if (UIIsWhiteSpace(token->content[contentOffset])) {
            if (0 != token->content[contentOffset]) {
                goto PARSE_HTML_DOM_TAG_END;
            }
            attributeDataPtr++;
        }

        isExpectingDoubleQuoteClose = 0;
        isExpectingQuoteClose = 0;
        while(1) {
            if ('\0' == *attributeDataPtr) {
                goto PARSE_ATTRIBUTE;
            }

            if ('\\' == *attributeDataPtr) {
                attributeDataPtr++;
                goto STORE_CHARACTER;
            }

            if (1 == isExpectingDoubleQuoteClose) {
                if ('"' == *attributeDataPtr) {
                    isExpectingDoubleQuoteClose = 0;
                    goto PARSE_NEXT_CHARACTER;

                } else {
                    goto STORE_CHARACTER;
                }
            }

            if (1 == isExpectingQuoteClose) {
                if ('\'' == *attributeDataPtr) {
                    isExpectingQuoteClose = 0;
                    goto PARSE_NEXT_CHARACTER;

                } else {
                    goto STORE_CHARACTER;
                }
            }

            // assert 0 == isExpectingDoubleQuoteClose
            // assert 0 == isExpectingQuoteClose
            if ('"' == *attributeDataPtr && 0 == isExpectingQuoteClose) {
                isExpectingDoubleQuoteClose = 1;
                goto PARSE_NEXT_CHARACTER;

            } else if ('\'' == *attributeDataPtr && 0 == isExpectingDoubleQuoteClose) {
                isExpectingQuoteClose = 1;
                goto PARSE_NEXT_CHARACTER;
            }

            if (UIIsWhiteSpace(*attributeDataPtr)) {
                attributeDataPtr++;
                break;
            }

            if ('=' == *attributeDataPtr) {
                attributeDataPtr++;
                break;
            }

STORE_CHARACTER:
            attributeData[attributeDataOffset] = *attributeDataPtr;
            attributeDataOffset++;

PARSE_NEXT_CHARACTER:
            attributeDataPtr++;
        }

PARSE_ATTRIBUTE:
        attributeData[attributeDataOffset] = '\0';

        switch(expectState) {
            case EXPECTING_KEY:
                attributeKey = stringnewlen(attributeData, strlen(attributeData));
                attributeDataOffset = 0;
                expectState = EXPECTING_VALUE;
                break;
            case EXPECTING_VALUE:
                attributeValue = stringnewlen(attributeData, strlen(attributeData));
                attributeDataOffset = 0;
                expectState = EXPECTING_KEY;

                if (0 == dom->attribute) {
                    dom->attribute = dictCreate(&stringTableDictType, 0);
                }
                dictAdd(dom->attribute, attributeKey, attributeValue);

                if (0 == stringcmp("class", attributeKey)) {
                    if (0 == dom->classes) {
                        dom->classes = listCreate();
                        dom->classes->free = listFreeSds;
                    }

                    sptr = attributeValue;
                    while(UIIsWhiteSpace(*sptr)) {
                        sptr++;
                    }
                    sptr2 = sptr + 1;
                    while(1) {
                        if ('\0' == *sptr2) {
                            break;
                        }

                        if (UIIsWhiteSpace(*sptr2)) {
                            listAddNodeTail(dom->classes, sdsnewlen(sptr, sptr2-sptr));

                            while(UIIsWhiteSpace(*sptr2)) {
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
                        listAddNodeTail(dom->classes, sdsnewlen(sptr, sptr2-sptr));
                    }

                } else if (0 == stringcmp("id", attributeKey)) {
                    if (0 != dom->id) {
                        sdsfree(dom->id);
                    }

                    dom->id = sdsnew(attributeValue);
                }

                attributeKey = 0;
                attributeValue = 0;
                break;
        }

        if (0 == *attributeDataPtr) {
            goto PARSE_HTML_DOM_TAG_END;
        }
    }

PARSE_HTML_DOM_TAG_END:
    if (0 != attributeData) sdsfree(attributeData);
    if (0 != attributeKey) zfree(attributeKey);
    if (0 != attributeValue) zfree(attributeValue);
}

// @return UIHtmlDom* 需要继续处理的 UIHtmlDom*

// <tag>
static inline UIHtmlDom* parseHtmlTokenStartTag(UIHtmlDom *dom, UIHtmlToken *token) {
    sdsrange(token->content, 1, -2);
    UIHtmlDom *newdom = UINewHtmlDom();
    newdom->parentDom = dom;
    parseHtmlDomTag(newdom, token);
    dom->children = listAddNodeTail(dom->children, newdom);
    return newdom;
}

// </tag>
static inline UIHtmlDom* parseHtmlTokenEndTag(UIHtmlDom *dom, UIHtmlToken *token) {
    sdsrange(token->content, 2, -2);
    return dom->parentDom;
}

// <tag/>
static inline UIHtmlDom* parseHtmlTokenSelfClosingTag(UIHtmlDom *dom, UIHtmlToken *token) {
    sdsrange(token->content, 1, -3);
    UIHtmlDom *newdom = UINewHtmlDom();
    newdom->parentDom = dom;
    parseHtmlDomTag(newdom, token);
    dom->children = listAddNodeTail(dom->children, newdom);
    return dom;
}

// text
static inline UIHtmlDom* parseHtmlTokenText(UIHtmlDom *dom, UIHtmlToken *token) {
    if (0 == stringcmp("script", dom->type)) {
        if (0 == dom->content) {
            dom->content = sdsnewlen(token->content, sdslen(token->content));
        } else {
            dom->content = sdscatsds(dom->content, token->content);
        }

    } else {
        // 除了 script 以外的dom内容，均作特殊处理
        UIHtmlDom *newdom = UINewHtmlDom();
        newdom->parentDom = dom;
        newdom->content = sdsempty();
        dom->children = listAddNodeTail(dom->children, newdom);

        newdom->type = "text";
        newdom->content = sdsMakeRoomFor(newdom->content, 
                sdslen(newdom->content)+sdslen(token->content));

        int domPoi = sdslen(newdom->content);
        int isWhiteSpaceFound = 0;
        int len = sdslen(token->content);
        char *ptr = token->content;
        char *ptr2;
        sds tmpString = sdsMakeRoomFor(sdsempty(), 12);
        for (int i = 0; i < len; i++,ptr++) {
            //合并空格
            if (UIIsWhiteSpace(*ptr)) {
                if (0 == isWhiteSpaceFound) {
                    isWhiteSpaceFound = 1;
                    newdom->content[domPoi] = ' ';
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
                while(i < len && ';' != *ptr2) {
                    i++;ptr2++;
                }
                sdsclear(tmpString);
                tmpString = sdscatlen(tmpString, ptr, ptr2-ptr+1);
                ptr = ptr2;
                ptr2 = dictFetchValue(UIHtmlSpecialStringTable, tmpString);
                if (0 != ptr2) {
                    newdom->content[domPoi] = *ptr2;
                    domPoi++;
                }
                continue;
            }

            newdom->content[domPoi] = *ptr;
            domPoi++;
        }
        sdsfree(tmpString);
        newdom->content[domPoi] = '\0';
        sdsupdatelen(newdom->content);
    }

    return dom;
}

UIHtmlDom* UIParseHtml(char *html) {
    UIHtmlDom *rootDom = UINewHtmlDom();
    UIHtmlDom *dom = rootDom;

    for (UIHtmlToken *token = UIHtmlNextToken(&html); 
            0 != token;
            token = UIHtmlNextToken(&html)) {

        switch(token->type) {
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
                dom = parseHtmlTokenText(dom, token);
                break;
        }
        UIHtmlFreeToken(token);
    }

    return rootDom;
}

void UIHtmlPrintDomTree(UIHtmlDom *dom, int indent) {
    int i;
    for (i = 0; i < indent; i++) { printf("  "); }
    printf("%s", dom->title);
    if (0 != dom->attribute && dictSize(dom->attribute) > 0) {
        dictEntry *de;
        dictIterator *di = dictGetIterator(dom->attribute);
        while ((de = dictNext(di)) != NULL) {
            printf(" %s=%s", dictGetKey(de), dictGetVal(de));
        }
        dictReleaseIterator(di);
    }

    if (0 == stringcmp("text", dom->type)) {
        printf("%s\n", dom->content);

    } else if (0 == stringcmp("script", dom->type)) {
        printf("\n");
        for (i = 0; i < indent; i++) { printf("  "); }
        printf("  %s\n", dom->content);

    } else {
        printf("\n");
    }

    listIter *li;
    listNode *ln;
    li = listGetIterator(dom->children, AL_START_HEAD);
    indent++;
    while (NULL != (ln = listNext(li))) {
        UIHtmlPrintDomTree((UIHtmlDom*)listNodeValue(ln), indent);
    }
    listReleaseIterator(li);
}
