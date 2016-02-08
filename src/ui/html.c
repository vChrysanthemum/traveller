#include "core/token.h"
#include "core/sds.h"
#include "ui/ui.h"

static inline void skipStringNotConcern(char **ptr)  {
    while( 0 != **ptr && (' ' == **ptr || '\t' == **ptr || '\r' == **ptr || '\n' == **ptr)) {
        (*ptr)++;
    }
}

static inline UIHTMLToken* newHtmlToken() {
    UIHTMLToken *token = (UIHTMLToken*)zmalloc(sizeof(UIHTMLToken));
    token->content = sdsempty();
    return token;
}

static inline void freeHtmlToken(UIHTMLToken *token) {
    sdsfree(token->content);
    zfree(token);
}

UIHTMLToken* UIHTMLNextToken(char **ptr) {
    if ('\0' == **ptr) {
        return 0;
    }

    int stack[6] = {0};
    int stackLen = 0;

    skipStringNotConcern(ptr);

    char *s = *ptr;
    
    UIHTMLToken *token = newHtmlToken();
    token->type = UIHTML_TOKEN_TEXT;

    printf("\n");
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
                    if (TOKEN_TEXT != stack[stackLen-1]) {
                        stack[stackLen] = TOKEN_TEXT;
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
                if (stackLen > 0 && TOKEN_TEXT != stack[stackLen-1]) {
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
                TOKEN_LSS == stack[0] &&
                TOKEN_TEXT == stack[0]) {
            //如果是: "text<"
            token->type = UIHTML_TOKEN_TEXT;
            stackLen--;
            goto GET_TOKEN_SUCCESS;
        }

        s++;
    }

GET_TOKEN_SUCCESS:
    token->content = sdscatlen(token->content, *ptr, s-*ptr+1);
    (*ptr) = s + 1;
    return token;
}

UIHTMLDom* UIHTMLGetNextDom(char *html) {
    return 0;
}
