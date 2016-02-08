#include "core/token.h"
#include "core/stack.h"
#include "core/sds.h"
#include "ui/ui.h"

static inline void skipStringNotConcern(char **ptr)  {
    while( 0 != **ptr && (' ' == **ptr || '\t' == **ptr || '\r' == **ptr || '\n' == **ptr)) {
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
            if (' ' == *s || '\t' == *s || '\r' == *s || '\n' == *s) {
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
    return 0;
}

void UIFreeHtmlDom(UIHtmlDom *dom) {
}

UIHtmlDom* UIHtmlNextDom(char *html) {
    for (UIHtmlToken *token = UIHtmlNextToken(&html); 0 != token;) {
        token = UIHtmlNextToken(&html);
        UIHtmlFreeToken(token);
    }
    return 0;
}
