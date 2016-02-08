#include "core/token.h"
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

    int stack[6] = {0};
    int stackLen = 0;

    char *s = *ptr;
    int len;
    
    UIHtmlToken *token = newHtmlToken();
    token->type = UIHTML_TOKEN_TEXT;

    while('\0' != *s && stackLen < 6) {
        // 解析覆盖一下情况
        // "<text>"
        // "<text/>"
        // "</text>"
        // "text"
        // "<text<text>"

        switch(*s) {
            case '<':
                //预防 "< lkasfd < >" 的情况
                if (0 == stackLen) {
                    stack[0] = TOKEN_LSS;
                    stackLen++;
                } else {
                    if (TOKEN_LSS == stack[0]) {
                        if (TOKEN_TEXT != stack[stackLen-1]) {
                            stack[stackLen] = TOKEN_TEXT;
                            stackLen++;
                        }
                    } else {
                        stack[stackLen] = TOKEN_LSS;
                        stackLen++;
                    }
                }
                break;
            case '>':
                stack[stackLen] = TOKEN_GTR;
                stackLen++;
                break;
            case '/':
                stack[stackLen] = TOKEN_QUO;
                stackLen++;
                break;
            default:
                if (0 == stackLen) {
                    stack[0] = TOKEN_TEXT;
                    stackLen++;
                } else if (TOKEN_TEXT != stack[stackLen-1]) {
                    stack[stackLen] = TOKEN_TEXT;
                    stackLen++;
                }
        }

        if (3 == stackLen && 
                TOKEN_LSS == stack[0]  &&   // <
                TOKEN_TEXT == stack[1] &&   // tag
                TOKEN_GTR == stack[2]) {    // >
            token->type = UIHTML_TOKEN_START_TAG;
            goto GET_TOKEN_SUCCESS;

        } else if (4 == stackLen) {
            if (TOKEN_LSS == stack[0]      && // <
                    TOKEN_QUO == stack[1]  && // /
                    TOKEN_TEXT == stack[2] && // tag
                    TOKEN_GTR == stack[3]) {    // >
                token->type = UIHTML_TOKEN_END_TAG;
                goto GET_TOKEN_SUCCESS;

            } else if (TOKEN_LSS == stack[0] && // <
                    TOKEN_TEXT == stack[1]   && // tag
                    TOKEN_QUO == stack[2]    && // /
                    TOKEN_GTR == stack[3]) {    // >
                token->type = UIHTML_TOKEN_SELF_CLOSING_TAG;
                goto GET_TOKEN_SUCCESS;
            }

        } else if (2 == stackLen && 
                TOKEN_TEXT == stack[0] &&    // text
                TOKEN_LSS == stack[1]) {     // <
            token->type = UIHTML_TOKEN_TEXT;
            stackLen--;
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
