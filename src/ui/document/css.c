#include "core/zmalloc.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/util.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

#include "core/extern.h"
#include "ui/extern.h"
#include "ui/document/extern.h"

dict *uiCssDeclarationInfoDict;
static uiCssDeclarationInfo_t uiCssDeclarationInfoTable[] = {
    {"background-color", UI_CSS_DECLARATION_TYPE_BACKGROUND_COLOR},
    {"color",            UI_CSS_DECLARATION_TYPE_COLOR},
    {"padding",          UI_CSS_DECLARATION_TYPE_PADDING},
    {"padding-top",      UI_CSS_DECLARATION_TYPE_PADDING_TOP},
    {"padding-bottom",   UI_CSS_DECLARATION_TYPE_PADDING_BOTTOM},
    {"padding-left",     UI_CSS_DECLARATION_TYPE_PADDING_LEFT},
    {"padding-right",    UI_CSS_DECLARATION_TYPE_PADDING_RIGHT},
    {"margin",           UI_CSS_DECLARATION_TYPE_MARGIN},
    {"margin-top",       UI_CSS_DECLARATION_TYPE_MARGIN_TOP},
    {"margin-bottom",    UI_CSS_DECLARATION_TYPE_MARGIN_BOTTOM},
    {"margin-left",      UI_CSS_DECLARATION_TYPE_MARGIN_LEFT},
    {"margin-right",     UI_CSS_DECLARATION_TYPE_MARGIN_RIGHT},
    {"display",          UI_CSS_DECLARATION_TYPE_DISPLAY},
    {"text-align",       UI_CSS_DECLARATION_TYPE_TEXT_ALIGN},
    {"width",            UI_CSS_DECLARATION_TYPE_WIDTH},
    {"height",           UI_CSS_DECLARATION_TYPE_HEIGHT},
    {"position",         UI_CSS_DECLARATION_TYPE_POSITION},
    {0},
};

static inline void skipStringNotConcern(char **ptr)  {
    while (UI_IsWhiteSpace(**ptr) && 0 != **ptr) {
        (*ptr)++;
    }
}

void UI_PrepareCss() {
    uiCssDeclarationInfoDict = dictCreate(&stackStringTableDictType, 0);
    for (uiCssDeclarationInfo_t *domInfo = &uiCssDeclarationInfoTable[0]; 0 != domInfo->name; domInfo++) {
        dictAdd(uiCssDeclarationInfoDict, domInfo->name, domInfo);
    }
}

uiCssDeclaration_t* UI_NewCssDeclaration() {
    uiCssDeclaration_t *cssDeclaration = (uiCssDeclaration_t*)zmalloc(sizeof(uiCssDeclaration_t));
    memset(cssDeclaration, 0, sizeof(uiCssDeclaration_t));
    cssDeclaration->type = UI_CSS_DECLARATION_TYPE_UNKNOWN;
    return cssDeclaration;
}

void UI_FreeCssDeclaration(void *_cssDeclaration) {
    uiCssDeclaration_t *cssDeclaration = (uiCssDeclaration_t*)_cssDeclaration;
    zfree(cssDeclaration);
}

void UI_ParseSdsToCssDeclaration(uiCssDeclaration_t *cssDeclaration, sds cssDeclarationKey, sds cssDeclarationValue) {
    cssDeclaration->key = cssDeclarationKey;
    cssDeclaration->value = cssDeclarationValue;
    uiCssDeclarationInfo_t *cssDeclarationInfo = dictFetchValue(uiCssDeclarationInfoDict, cssDeclaration->key);
    if (0 != cssDeclarationInfo) {
        cssDeclaration->type = cssDeclarationInfo->type;
    }
}

void UI_CompileCssDeclarations(list **cssDeclarations, char *code) {
    enum {EXPECTING_KEY, EXPECTING_VALUE} expectState;

    sds data = sdsMakeRoomFor(sdsempty(), strlen(code));
    uiCssDeclaration_t *cssDeclaration;
    sds cssDeclarationKey = 0;
    sds cssDeclarationValue = 0;

    expectState = EXPECTING_KEY;
    int offset;
    while (1) {
        if (UI_IsWhiteSpace(*code)) {
            code++;
        }
        
        if ('\'' == *code || '"' == *code) {
            escapeQuoteContent(data, &code);

        } else {
            offset = -1;
            code--;
            while(1) {
                code++;
                offset++;
                if ('\0' == *code) {
                    break;
                }

                if (UI_IsWhiteSpace(*code) ||
                        ';' == *code ||
                        ':' == *code) {
                    code++;
                    break;
                } 

                data[offset] = *code;
            }
            data[offset] = '\0';
        }


        switch (expectState) {
            case EXPECTING_KEY:
                expectState = EXPECTING_VALUE;
                cssDeclarationKey = sdsnewlen(data, strlen(data));
                break;

            case EXPECTING_VALUE:
                expectState = EXPECTING_KEY;
                cssDeclarationValue = sdsnewlen(data, strlen(data));

                if (0 == *cssDeclarations) {
                    *cssDeclarations = listCreate();
                    (*cssDeclarations)->free = UI_FreeCssDeclaration;
                }
                cssDeclaration = UI_NewCssDeclaration();
                UI_ParseSdsToCssDeclaration(cssDeclaration, cssDeclarationKey, cssDeclarationValue);
                *cssDeclarations = listAddNodeTail(*cssDeclarations, cssDeclaration);

                cssDeclarationKey = 0;
                cssDeclarationValue = 0;

                break;
        }

        if ('\0' == *code) {
            break;
        }
    }

    if (0 != cssDeclarationKey) sdsfree(cssDeclarationKey);
    if (0 != cssDeclarationValue) sdsfree(cssDeclarationValue);
    sdsfree(data);
}

void UI_CompileCssSelector(uiCssSelector_t **selector, char *code) {
    if (0 == *selector) {
        *selector = UI_NewCssSelector();
    }

    uiCssSelectorSection_t *section;

    int offset = 0;
    int isSelectorSectionAttributeFound;
    int selectorSectionAttributeIndex;
    int selectorSectionAttributeEnd;
    int selectorSectionValueIndex;
    int selectorSectionValueEnd;
    while (1) {
        if ('\0' == code[offset]) {
            break;
        }

        while (UI_IsWhiteSpace(code[offset])) {
            offset++;
        }

        selectorSectionAttributeIndex = offset+1;
        isSelectorSectionAttributeFound = 0;
        section = UI_NewCssSelectorSection();
        section->attributeType = UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE;
        while (1) {
            if ('\0' == code[selectorSectionAttributeIndex] || UI_IsWhiteSpace(code[selectorSectionAttributeIndex])) {
                break;
            }

            if ('.' == code[selectorSectionAttributeIndex]) {
                section->attributeType = UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_CLASS;

                selectorSectionAttributeIndex++;
                selectorSectionAttributeEnd = selectorSectionAttributeIndex;
                while(1) {
                    if (UI_IsWhiteSpace(code[selectorSectionAttributeEnd]) || '\0' == code[selectorSectionAttributeEnd]) {
                        break;
                    }
                    selectorSectionAttributeEnd++;
                }

                section->attribute = sdsnewlen(&code[selectorSectionAttributeIndex], 
                        selectorSectionAttributeEnd-selectorSectionAttributeIndex);
                isSelectorSectionAttributeFound = 1;
                break;
            }

            selectorSectionAttributeIndex++;
        }

        if ('#' == code[offset]) {
            section->type = UI_SELECTOR_SECTION_TYPE_ID;
            selectorSectionValueIndex = offset + 1;
        } else if ('.' == code[offset]) {
            section->type = UI_SELECTOR_SECTION_TYPE_CLASS;
            selectorSectionValueIndex = offset + 1;
        } else {
            section->type = UI_SELECTOR_SECTION_TYPE_TAG;
            selectorSectionValueIndex = offset;
        }

        if (1 == isSelectorSectionAttributeFound) {
            selectorSectionValueEnd = selectorSectionAttributeIndex - 1;
            section->value = sdsnewlen(&code[selectorSectionValueIndex],
                    selectorSectionValueEnd-selectorSectionValueIndex);
            offset = selectorSectionAttributeEnd;
        } else {
            selectorSectionValueEnd = selectorSectionValueIndex;
            while(1) {
                if (UI_IsWhiteSpace(code[selectorSectionValueEnd]) || '\0' == code[selectorSectionValueEnd]) {
                    break;
                }
                selectorSectionValueEnd++;
            }
            section->value = sdsnewlen(&code[selectorSectionValueIndex],
                    selectorSectionValueEnd-selectorSectionValueIndex);
            offset = selectorSectionValueEnd;
        }

        (*selector)->sections = listAddNodeTail((*selector)->sections, section);
    }
}

void UI_UpdateCssDeclaration(uiCssDeclaration_t *src, uiCssDeclaration_t *dst) {
    src->type = dst->type;
    src->key = sdsupdate(src->key, dst->key);
    src->value = sdsupdate(src->value, dst->value);
}

uiCssDeclaration_t* UI_DuplicateCssDeclaration(uiCssDeclaration_t *dst) {
    uiCssDeclaration_t *result = UI_NewCssDeclaration();
    UI_UpdateCssDeclaration(result, dst);
    return result;
}

uiCssDeclarationList_t* UI_DuplicateCssDeclarationList(uiCssDeclarationList_t* cssDeclarationList) {
    cssDeclarationList->referenceCount++;
    return cssDeclarationList;
}

uiCssDeclarationList_t* UI_NewCssDeclarationList() {
    uiCssDeclarationList_t *cssDeclarationList = (uiCssDeclarationList_t*)zmalloc(sizeof(uiCssDeclarationList_t));
    cssDeclarationList->referenceCount = 1;
    cssDeclarationList->data = listCreate();
    cssDeclarationList->data->free = UI_FreeCssDeclaration;
    return cssDeclarationList;
}

void UI_FreeCssDeclarationList(uiCssDeclarationList_t *cssDeclarationList) {
    cssDeclarationList--;
    if (cssDeclarationList < 0) {
        listRelease(cssDeclarationList->data);
        zfree(cssDeclarationList);
    }
}

uiCssSelectorSection_t* UI_NewCssSelectorSection() {
    uiCssSelectorSection_t *section = (uiCssSelectorSection_t*)zmalloc(sizeof(uiCssSelectorSection_t));
    memset(section, 0, sizeof(uiCssSelectorSection_t));
    section->type = UI_SELECTOR_SECTION_TYPE_UNKNOWN;
    return section;
}

void UI_FreeCssSelectorSection(void *_section) {
    uiCssSelectorSection_t *section = (uiCssSelectorSection_t*)_section;
    sdsfree(section->value);
    zfree(section);
}

uiCssSelector_t* UI_NewCssSelector() {
    uiCssSelector_t *selector = (uiCssSelector_t*)zmalloc(sizeof(uiCssSelector_t));
    memset(selector, 0, sizeof(uiCssSelector_t));
    selector->sections = listCreate();
    selector->sections->free = UI_FreeCssSelectorSection;
    return selector;
}

void UI_FreeCssSelector(uiCssSelector_t *selector) {
    zfree(selector);
}

uiCssRule_t* UI_NewCssRule() {
    uiCssRule_t* rule = (uiCssRule_t*)zmalloc(sizeof(uiCssRule_t));
    memset(rule, 0, sizeof(uiCssRule_t));
    return rule;
}

void UI_FreeCssRule(void *_rule) {
    uiCssRule_t *rule = (uiCssRule_t*)_rule;
    UI_FreeCssSelector(rule->selector);
    UI_FreeCssDeclarationList(rule->cssDeclarationList);
    zfree(rule);
}

uiCssStyleSheet_t* UI_NewCssStyleSheet() {
    uiCssStyleSheet_t* cssStyleSheet = (uiCssStyleSheet_t*)zmalloc(sizeof(uiCssStyleSheet_t));
    memset(cssStyleSheet, 0, sizeof(uiCssStyleSheet_t));
    cssStyleSheet->rules = listCreate();
    cssStyleSheet->rules->free = UI_FreeCssRule;
    return cssStyleSheet;
}

uiDocumentScanToken_t* UI_ScanCssToken(uiDocumentScanner_t *scanner) {
    char *ptr = scanner->current;
    skipStringNotConcern(&ptr);

    if ('\0' == *ptr) return 0;

    uiDocumentScanToken_t *token = (uiDocumentScanToken_t*)zmalloc(sizeof(uiDocumentScanToken_t));
    token->content = 0;

    if (',' == *ptr) token->type = UI_TOKEN_COMMA;
    else if (':' == *ptr) token->type = UI_TOKEN_COLON;
    else if (';' == *ptr) token->type = UI_TOKEN_SEMICOLON;
    else if ('{' == *ptr) token->type = UI_TOKEN_BLOCK_START;
    else if ('}' == *ptr) token->type = UI_TOKEN_BLOCK_END;
    else {
        token->type = UI_TOKEN_TEXT;
        char *startPtr = ptr;
        for (; !UI_IsWhiteSpace(*ptr) &&
                ',' != *ptr &&
                ':' != *ptr &&
                ';' != *ptr &&
                '{' != *ptr &&
                '}' != *ptr; ptr++) {
        }

        token->content = sdsnewlen(startPtr, ptr-startPtr);
        ptr--;
    }

    ptr++;

    scanner->current = ptr;

    return token;
}

void UI_CssStyleSheetMergeRule(uiCssStyleSheet_t *cssStyleSheet, uiCssRule_t *rule) {
    cssStyleSheet->rules = listAddNodeTail(cssStyleSheet->rules, rule);
}

static int parseCssTokenText(uiDocumentScanner_t *scanner, uiDocumentScanToken_t *token,
        uiCssSelector_t **selector, uiCssDeclaration_t **cssDeclaration) {
    switch (scanner->state) {
        case UI_PARSE_STATE_SELECTOR:
            UI_CompileCssSelector(selector, token->content);
            break;

        case UI_PARSE_STATE_CSS_DECLARATION_KEY:
            *cssDeclaration = UI_NewCssDeclaration();
            (*cssDeclaration)->key = sdsnewlen(token->content, sdslen(token->content));
            uiCssDeclarationInfo_t *cssDeclarationInfo = dictFetchValue(uiCssDeclarationInfoDict, token->content);
            if (0 != cssDeclarationInfo) {
                (*cssDeclaration)->type = cssDeclarationInfo->type;
            }
            break;

        case UI_PARSE_STATE_CSS_DECLARATION_VALUE:
            (*cssDeclaration)->value = sdsnewlen(token->content, sdslen(token->content));
            break;
    }
    return 0;
}

const char* UI_ParseCssStyleSheet(uiDocument_t *document, char *cssContent) {
    uiDocumentScanner_t cssScanner = {
        UI_PARSE_STATE_SELECTOR, cssContent, cssContent, UI_ScanCssToken
    };

    uiDocumentScanToken_t *token;
    uiCssSelector_t *selector = 0;
    uiCssDeclaration_t *cssDeclaration = 0;
    list *selectors = listCreate();
    uiCssDeclarationList_t* cssDeclarationList = UI_NewCssDeclarationList();
    while (0 != (token = cssScanner.scan(&cssScanner))) {
        switch (token->type) {
            case UI_TOKEN_COMMA:
                AssertOrReturnError((UI_PARSE_STATE_SELECTOR == cssScanner.state),
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                if (0 != selector) {
                    selectors = listAddNodeTail(selectors, selector);
                    selector = 0;
                }
                break;

            case UI_TOKEN_COLON:
                AssertOrReturnError((UI_PARSE_STATE_CSS_DECLARATION_KEY == cssScanner.state),
                        UIERROR_CSS_PARSE_STATE_NOT_CSS_DECLARATION_KEY);

                cssScanner.state = UI_PARSE_STATE_CSS_DECLARATION_VALUE;
                break;

            case UI_TOKEN_SEMICOLON:
                AssertOrReturnError((UI_PARSE_STATE_CSS_DECLARATION_VALUE == cssScanner.state),
                        UIERROR_CSS_PARSE_STATE_NOT_CSS_DECLARATION_VALUE);

                if (0 != cssDeclaration) {
                    cssDeclarationList->data = listAddNodeTail(cssDeclarationList->data, cssDeclaration);
                    cssDeclaration = 0;
                }
                cssScanner.state = UI_PARSE_STATE_CSS_DECLARATION_KEY;
                break;

            case UI_TOKEN_BLOCK_START:
                AssertOrReturnError((UI_PARSE_STATE_SELECTOR == cssScanner.state),
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                if (0 != selector) {
                    selectors = listAddNodeTail(selectors, selector);
                    selector = 0;
                }
                cssScanner.state = UI_PARSE_STATE_CSS_DECLARATION_KEY;
                break;

            case UI_TOKEN_BLOCK_END:
                if (0 != cssDeclaration) {
                    cssDeclarationList->data = listAddNodeTail(cssDeclarationList->data, cssDeclaration);
                    cssDeclaration = 0;
                }
                cssScanner.state = UI_PARSE_STATE_SELECTOR;
                break;

            case UI_TOKEN_TEXT:
                parseCssTokenText(&cssScanner, token, &selector, &cssDeclaration);
                break;
        }

        UI_FreeDocumentScanToken(token);
    }


    uiCssRule_t *rule;

    listIter *li;
    listNode *ln;
    li = listGetIterator(selectors, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        rule = UI_NewCssRule();
        rule->selector = listNodeValue(ln);
        rule->cssDeclarationList = UI_DuplicateCssDeclarationList(cssDeclarationList);
        UI_CssStyleSheetMergeRule(document->cssStyleSheet, rule);
    }
    listReleaseIterator(li);

    listRelease(selectors);
    UI_FreeCssDeclarationList(cssDeclarationList);

    return 0;
}


void UI_PrintCssStyleSheet(uiCssStyleSheet_t *cssStyleSheet) {
    uiCssRule_t *rule;
    uiCssSelectorSection_t *selectorSection;
    uiCssDeclaration_t *cssDeclaration;

    listIter *liSelectorSection;
    listNode *lnSelectorSection;

    listIter *liCssDeclaration;
    listNode *lnCssDeclaration;

    listIter *li;
    listNode *ln;
    li = listGetIterator(cssStyleSheet->rules, AL_START_HEAD);
    printf("\n");
    while (0 != (ln = listNext(li))) {
        rule = (uiCssRule_t*)listNodeValue(ln);

        liSelectorSection = listGetIterator(rule->selector->sections, AL_START_HEAD);
        while (0 != (lnSelectorSection = listNext(liSelectorSection))) {
            selectorSection = (uiCssSelectorSection_t*)listNodeValue(lnSelectorSection);

            printf("%s ", selectorSection->value);

            if (UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE != selectorSection->attributeType) {
                printf("%d %s", selectorSection->attributeType, selectorSection->attribute);
            }
        }
        listReleaseIterator(liSelectorSection);

        printf(" {");
        liCssDeclaration = listGetIterator(rule->cssDeclarationList->data, AL_START_HEAD);
        while (0 != (lnCssDeclaration = listNext(liCssDeclaration))) {
            cssDeclaration = (uiCssDeclaration_t*)listNodeValue(lnCssDeclaration);

            printf("%d %s:%s; ", cssDeclaration->type, cssDeclaration->key, cssDeclaration->value);
        }
        listReleaseIterator(liCssDeclaration);
        printf("} ");
        printf("\n");
    }
    listReleaseIterator(li);
}
