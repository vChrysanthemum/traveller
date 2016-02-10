#include "core/token.h"
#include "core/stack.h"
#include "core/sds.h"
#include "ui/ui.h"

#include "core/extern.h"

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
                    tokenStack[0] = TOKEN_LSS;
                    tokenStackLen++;
                } else {
                    if (TOKEN_LSS == tokenStack[0]) {
                        if (TOKEN_TEXT != tokenStack[tokenStackLen-1]) {
                            tokenStack[tokenStackLen] = TOKEN_TEXT;
                            tokenStackLen++;
                        }
                    } else {
                        tokenStack[tokenStackLen] = TOKEN_LSS;
                        tokenStackLen++;
                    }
                }
                break;
            case '>':
                tokenStack[tokenStackLen] = TOKEN_GTR;
                tokenStackLen++;
                break;
            case '/':
                tokenStack[tokenStackLen] = TOKEN_QUO;
                tokenStackLen++;
                break;
            default:
                if (0 == tokenStackLen) {
                    tokenStack[0] = TOKEN_TEXT;
                    tokenStackLen++;
                } else if (TOKEN_TEXT != tokenStack[tokenStackLen-1]) {
                    tokenStack[tokenStackLen] = TOKEN_TEXT;
                    tokenStackLen++;
                }
        }

        if (3 == tokenStackLen && 
                TOKEN_LSS == tokenStack[0]  &&   // <
                TOKEN_TEXT == tokenStack[1] &&   // tag
                TOKEN_GTR == tokenStack[2]) {    // >
            token->type = UIHTML_TOKEN_START_TAG;
            goto GET_TOKEN_SUCCESS;

        } else if (4 == tokenStackLen) {
            if (TOKEN_LSS == tokenStack[0]      && // <
                    TOKEN_QUO == tokenStack[1]  && // /
                    TOKEN_TEXT == tokenStack[2] && // tag
                    TOKEN_GTR == tokenStack[3]) {    // >
                token->type = UIHTML_TOKEN_END_TAG;
                goto GET_TOKEN_SUCCESS;

            } else if (TOKEN_LSS == tokenStack[0] && // <
                    TOKEN_TEXT == tokenStack[1]   && // tag
                    TOKEN_QUO == tokenStack[2]    && // /
                    TOKEN_GTR == tokenStack[3]) {    // >
                token->type = UIHTML_TOKEN_SELF_CLOSING_TAG;
                goto GET_TOKEN_SUCCESS;
            }

        } else if (2 == tokenStackLen && 
                TOKEN_TEXT == tokenStack[0] &&    // text
                TOKEN_LSS == tokenStack[1]) {     // <
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
    dom->children = listCreate();
    dom->children->free = UIFreeHtmlDom;
    return dom;
}

void UIFreeHtmlDom(void *_dom) {
    UIHtmlDom *dom = (UIHtmlDom*)_dom;
    sdsfree(dom->content);
    sdsfree(dom->title);
    listRelease(dom->children);
}

// 解析单个tag 中的 title和attribute
static inline void parseHtmlDomTag(UIHtmlDom *dom, UIHtmlToken *token) {
    int contentLen = sdslen(token->content);

    int contentOffset;
    int contentEndOffset;
    char *attributeKey = 0;
    char *attributeValue = 0;
    enum {EXPECTING_KEY, EXPECTING_VALUE} expectState;
    int isExpectingDoubleQuoteClose, isExpectingQuoteClose;

    dom->attribute = dictCreate(&stringTableDictType, 0);

    // 提取 title
    contentOffset = 0;
    for (contentEndOffset = 1; contentEndOffset < contentLen; contentEndOffset++) {
        if (UIIsWhiteSpace(token->content[contentEndOffset])) {
            break;
        }
    }
    dom->title = sdscatlen(dom->title, &(token->content[contentOffset]), contentEndOffset-contentOffset);

    contentOffset = contentEndOffset + 1;

    // 提取attribute
    expectState = EXPECTING_KEY;
    while(1) {

        // 跳过空格
        if (UIIsWhiteSpace(token->content[contentOffset])) {
            if (0 != token->content[contentOffset]) {
                goto PARSE_HTML_DOM_TAG_END;
            }
            contentOffset++;
        }
        contentEndOffset = contentOffset + 1;

        isExpectingDoubleQuoteClose = 0;
        isExpectingQuoteClose = 0;
        while(1) {
            if (0 == token->content[contentEndOffset]) {
                goto PARSE_HTML_DOM_TAG_END;
            }

            if ('\\' == token->content[contentEndOffset]) {
                goto PARSE_NEXT_CHARACTER;
            }

            if (1 == isExpectingDoubleQuoteClose) {
                if ('"' == token->content[contentEndOffset]) {
                    isExpectingDoubleQuoteClose = 0;
                }
                goto PARSE_NEXT_CHARACTER;
            }

            if (1 == isExpectingQuoteClose) {
                if ('\'' == token->content[contentEndOffset]) {
                    isExpectingQuoteClose = 0;
                }
                goto PARSE_NEXT_CHARACTER;
            }

            if ('"' == token->content[contentEndOffset]) {
                isExpectingDoubleQuoteClose = 1;
                goto PARSE_NEXT_CHARACTER;

            } else if ('\'' == token->content[contentEndOffset]) {
                isExpectingQuoteClose = 1;
                goto PARSE_NEXT_CHARACTER;
            }

            if (UIIsWhiteSpace(token->content[contentEndOffset])) {
                break;
            }
            if ('=' == token->content[contentEndOffset]) {
                break;
            }

PARSE_NEXT_CHARACTER:
            contentEndOffset++;
        }

        switch(expectState) {
            case EXPECTING_KEY:
                attributeKey = stringnewlen(&(token->content[contentOffset]), contentEndOffset-contentOffset);
                contentOffset = contentEndOffset + 1;
                expectState = EXPECTING_VALUE;
                break;
            case EXPECTING_VALUE:
                attributeValue = stringnewlen(&(token->content[contentOffset]), contentEndOffset-contentOffset);
                contentOffset = contentEndOffset + 1;
                dictAdd(dom->attribute, attributeKey, attributeValue);
                expectState = EXPECTING_KEY;
                break;
        }
    }

PARSE_HTML_DOM_TAG_END:
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
    if (0 == dom->content) {
        dom->content = sdsempty();
    }

    if (0 == stringcmp("script", dom->title)) {
        dom->content = sdscatsds(dom->content, token->content);
        
    } else {
        // 除了 script 以外的dom内容，均合并多个空格为一个空格
        dom->content = sdsMakeRoomFor(dom->content, sdslen(dom->content)+sdslen(token->content));

        int domPoi = sdslen(dom->content);
        int isWhiteSpaceFound = 0;
        int len = sdslen(token->content);
        char *ptr = token->content;
        for (int i = 0; i < len; i++,ptr++) {
            if (UIIsWhiteSpace(*ptr)) {
                if (0 == isWhiteSpaceFound) {
                    isWhiteSpaceFound = 1;
                    dom->content[domPoi] = ' ';
                    domPoi++;
                }

            } else {
                if (1 == isWhiteSpaceFound) {
                    isWhiteSpaceFound = 0;
                }
                dom->content[domPoi] = *ptr;
                domPoi++;
            }
        }
        dom->content[domPoi] = '\0';
    }

    sdsupdatelen(dom->content);
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
    printf("%s\n", dom->title);
    if (0 != dom->content) {
        for (i = 0; i < indent; i++) { printf("  "); }
        printf("  %s\n", dom->content);
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
