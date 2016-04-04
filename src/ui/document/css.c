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
    {"Padding",          UI_CSS_DECLARATION_TYPE_PADDING},
    {"padding-top",      UI_CSS_DECLARATION_TYPE_PADDING_TOP},
    {"padding-bottom",   UI_CSS_DECLARATION_TYPE_PADDING_BOTTOM},
    {"padding-left",     UI_CSS_DECLARATION_TYPE_PADDING_LEFT},
    {"padding-right",    UI_CSS_DECLARATION_TYPE_PADDING_RIGHT},
    {"Margin",           UI_CSS_DECLARATION_TYPE_MARGIN},
    {"margin-top",       UI_CSS_DECLARATION_TYPE_MARGIN_TOP},
    {"margin-bottom",    UI_CSS_DECLARATION_TYPE_MARGIN_BOTTOM},
    {"margin-left",      UI_CSS_DECLARATION_TYPE_MARGIN_LEFT},
    {"margin-right",     UI_CSS_DECLARATION_TYPE_MARGIN_RIGHT},
    {"display",          UI_CSS_DECLARATION_TYPE_DISPLAY},
    {"text-align",       UI_CSS_DECLARATION_TYPE_TEXT_ALIGN},
    {"width",            UI_CSS_DECLARATION_TYPE_WIDTH},
    {"height",           UI_CSS_DECLARATION_TYPE_HEIGHT},
    {"position",         UI_CSS_DECLARATION_TYPE_POSITION},
    {"left",             UI_CSS_DECLARATION_TYPE_LEFT},
    {"right",            UI_CSS_DECLARATION_TYPE_RIGHT},
    {"top",              UI_CSS_DECLARATION_TYPE_TOP},
    {"bottom",           UI_CSS_DECLARATION_TYPE_BOTTOM},
    {0,0},
};

static inline void skipStringNotConcern(char **ptr)  {
    while (UI_IsWhiteSpace(**ptr) && 0 != **ptr) {
        (*ptr)++;
    }
}

void UI_PrepareCss() {
    uiCssDeclarationInfoDict = dictCreate(&stackStringTableDictType, 0);
    for (uiCssDeclarationInfo_t *domInfo = &uiCssDeclarationInfoTable[0]; 0 != domInfo->Name; domInfo++) {
        dictAdd(uiCssDeclarationInfoDict, domInfo->Name, domInfo);
    }
}

uiCssDeclaration_t* UI_NewCssDeclaration() {
    uiCssDeclaration_t *cssDeclaration = (uiCssDeclaration_t*)zmalloc(sizeof(uiCssDeclaration_t));
    memset(cssDeclaration, 0, sizeof(uiCssDeclaration_t));
    cssDeclaration->Type = UI_CSS_DECLARATION_TYPE_UNKNOWN;
    return cssDeclaration;
}

void UI_FreeCssDeclaration(void *_cssDeclaration) {
    uiCssDeclaration_t *cssDeclaration = (uiCssDeclaration_t*)_cssDeclaration;
    zfree(cssDeclaration);
}

void UI_ParseSdsToCssDeclaration(uiCssDeclaration_t *cssDeclaration, sds cssDeclarationKey, sds cssDeclarationValue) {
    cssDeclaration->Key = cssDeclarationKey;
    cssDeclaration->Value = cssDeclarationValue;
    uiCssDeclarationInfo_t *cssDeclarationInfo = dictFetchValue(uiCssDeclarationInfoDict, cssDeclaration->Key);
    if (0 != cssDeclarationInfo) {
        cssDeclaration->Type = cssDeclarationInfo->Type;
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
        section->AttributeType = UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE;
        while (1) {
            if ('\0' == code[selectorSectionAttributeIndex] || UI_IsWhiteSpace(code[selectorSectionAttributeIndex])) {
                break;
            }

            if ('.' == code[selectorSectionAttributeIndex]) {
                section->AttributeType = UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_CLASS;

                selectorSectionAttributeIndex++;
                selectorSectionAttributeEnd = selectorSectionAttributeIndex;
                while(1) {
                    if (UI_IsWhiteSpace(code[selectorSectionAttributeEnd]) || '\0' == code[selectorSectionAttributeEnd]) {
                        break;
                    }
                    selectorSectionAttributeEnd++;
                }

                section->Attribute = sdsnewlen(&code[selectorSectionAttributeIndex], 
                        selectorSectionAttributeEnd-selectorSectionAttributeIndex);
                isSelectorSectionAttributeFound = 1;
                break;
            }

            selectorSectionAttributeIndex++;
        }

        if ('#' == code[offset]) {
            section->Type = UI_SELECTOR_SECTION_TYPE_ID;
            selectorSectionValueIndex = offset + 1;
        } else if ('.' == code[offset]) {
            section->Type = UI_SELECTOR_SECTION_TYPE_CLASS;
            selectorSectionValueIndex = offset + 1;
        } else {
            section->Type = UI_SELECTOR_SECTION_TYPE_TAG;
            selectorSectionValueIndex = offset;
        }

        if (1 == isSelectorSectionAttributeFound) {
            selectorSectionValueEnd = selectorSectionAttributeIndex - 1;
            section->Value = sdsnewlen(&code[selectorSectionValueIndex],
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
            section->Value = sdsnewlen(&code[selectorSectionValueIndex],
                    selectorSectionValueEnd-selectorSectionValueIndex);
            offset = selectorSectionValueEnd;
        }

        (*selector)->Sections = listAddNodeTail((*selector)->Sections, section);
    }
}

void UI_UpdateCssDeclaration(uiCssDeclaration_t *src, uiCssDeclaration_t *dst) {
    src->Type = dst->Type;
    src->Key = sdsupdate(src->Key, dst->Key);
    src->Value = sdsupdate(src->Value, dst->Value);
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
    cssDeclarationList->Data = listCreate();
    cssDeclarationList->Data->free = UI_FreeCssDeclaration;
    return cssDeclarationList;
}

void UI_FreeCssDeclarationList(uiCssDeclarationList_t *cssDeclarationList) {
    cssDeclarationList--;
    if (cssDeclarationList < 0) {
        listRelease(cssDeclarationList->Data);
        zfree(cssDeclarationList);
    }
}

uiCssSelectorSection_t* UI_NewCssSelectorSection() {
    uiCssSelectorSection_t *section = (uiCssSelectorSection_t*)zmalloc(sizeof(uiCssSelectorSection_t));
    memset(section, 0, sizeof(uiCssSelectorSection_t));
    section->Type = UI_SELECTOR_SECTION_TYPE_UNKNOWN;
    return section;
}

void UI_FreeCssSelectorSection(void *_section) {
    uiCssSelectorSection_t *section = (uiCssSelectorSection_t*)_section;
    sdsfree(section->Value);
    zfree(section);
}

uiCssSelector_t* UI_NewCssSelector() {
    uiCssSelector_t *selector = (uiCssSelector_t*)zmalloc(sizeof(uiCssSelector_t));
    memset(selector, 0, sizeof(uiCssSelector_t));
    selector->Sections = listCreate();
    selector->Sections->free = UI_FreeCssSelectorSection;
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
    UI_FreeCssSelector(rule->Selector);
    UI_FreeCssDeclarationList(rule->CssDeclarationList);
    zfree(rule);
}

uiCssStyleSheet_t* UI_NewCssStyleSheet() {
    uiCssStyleSheet_t* cssStyleSheet = (uiCssStyleSheet_t*)zmalloc(sizeof(uiCssStyleSheet_t));
    memset(cssStyleSheet, 0, sizeof(uiCssStyleSheet_t));
    cssStyleSheet->Rules = listCreate();
    cssStyleSheet->Rules->free = UI_FreeCssRule;
    return cssStyleSheet;
}

void UI_FreeCssStyleSheet(uiCssStyleSheet_t *cssStyleSheet) {
    listRelease(cssStyleSheet->Rules);
    zfree(cssStyleSheet);
}

uiDocumentScanToken_t* UI_ScanCssToken(uiDocumentScanner_t *scanner) {
    char *ptr = scanner->Current;
    skipStringNotConcern(&ptr);

    if ('\0' == *ptr) return 0;

    uiDocumentScanToken_t *token = (uiDocumentScanToken_t*)zmalloc(sizeof(uiDocumentScanToken_t));
    token->Content = 0;

    if (',' == *ptr) token->Type = UI_TOKEN_COMMA;
    else if (':' == *ptr) token->Type = UI_TOKEN_COLON;
    else if (';' == *ptr) token->Type = UI_TOKEN_SEMICOLON;
    else if ('{' == *ptr) token->Type = UI_TOKEN_BLOCK_START;
    else if ('}' == *ptr) token->Type = UI_TOKEN_BLOCK_END;
    else {
        token->Type = UI_TOKEN_TEXT;
        char *startPtr = ptr;
        for (; !UI_IsWhiteSpace(*ptr) &&
                ',' != *ptr &&
                ':' != *ptr &&
                ';' != *ptr &&
                '{' != *ptr &&
                '}' != *ptr; ptr++) {
        }

        token->Content = sdsnewlen(startPtr, ptr-startPtr);
        ptr--;
    }

    ptr++;

    scanner->Current = ptr;

    return token;
}

void UI_CssStyleSheetMergeRule(uiCssStyleSheet_t *cssStyleSheet, uiCssRule_t *rule) {
    cssStyleSheet->Rules = listAddNodeTail(cssStyleSheet->Rules, rule);
}

static int parseCssTokenText(uiDocumentScanner_t *scanner, uiDocumentScanToken_t *token,
        uiCssSelector_t **selector, uiCssDeclaration_t **cssDeclaration) {
    switch (scanner->State) {
        case UI_PARSE_STATE_SELECTOR:
            UI_CompileCssSelector(selector, token->Content);
            break;

        case UI_PARSE_STATE_CSS_DECLARATION_KEY:
            *cssDeclaration = UI_NewCssDeclaration();
            (*cssDeclaration)->Key = sdsnewlen(token->Content, sdslen(token->Content));
            uiCssDeclarationInfo_t *cssDeclarationInfo = dictFetchValue(uiCssDeclarationInfoDict, token->Content);
            if (0 != cssDeclarationInfo) {
                (*cssDeclaration)->Type = cssDeclarationInfo->Type;
            }
            break;

        case UI_PARSE_STATE_CSS_DECLARATION_VALUE:
            (*cssDeclaration)->Value = sdsnewlen(token->Content, sdslen(token->Content));
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
    while (0 != (token = cssScanner.Scan(&cssScanner))) {
        switch (token->Type) {
            case UI_TOKEN_COMMA:
                AssertOrReturnError((UI_PARSE_STATE_SELECTOR == cssScanner.State),
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                if (0 != selector) {
                    selectors = listAddNodeTail(selectors, selector);
                    selector = 0;
                }
                break;

            case UI_TOKEN_COLON:
                AssertOrReturnError((UI_PARSE_STATE_CSS_DECLARATION_KEY == cssScanner.State),
                        UIERROR_CSS_PARSE_STATE_NOT_CSS_DECLARATION_KEY);

                cssScanner.State = UI_PARSE_STATE_CSS_DECLARATION_VALUE;
                break;

            case UI_TOKEN_SEMICOLON:
                AssertOrReturnError((UI_PARSE_STATE_CSS_DECLARATION_VALUE == cssScanner.State),
                        UIERROR_CSS_PARSE_STATE_NOT_CSS_DECLARATION_VALUE);

                if (0 != cssDeclaration) {
                    cssDeclarationList->Data = listAddNodeTail(cssDeclarationList->Data, cssDeclaration);
                    cssDeclaration = 0;
                }
                cssScanner.State = UI_PARSE_STATE_CSS_DECLARATION_KEY;
                break;

            case UI_TOKEN_BLOCK_START:
                AssertOrReturnError((UI_PARSE_STATE_SELECTOR == cssScanner.State),
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                if (0 != selector) {
                    selectors = listAddNodeTail(selectors, selector);
                    selector = 0;
                }
                cssScanner.State = UI_PARSE_STATE_CSS_DECLARATION_KEY;
                break;

            case UI_TOKEN_BLOCK_END:
                if (0 != cssDeclaration) {
                    cssDeclarationList->Data = listAddNodeTail(cssDeclarationList->Data, cssDeclaration);
                    cssDeclaration = 0;
                }
                cssScanner.State = UI_PARSE_STATE_SELECTOR;
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
        rule->Selector = listNodeValue(ln);
        rule->CssDeclarationList = UI_DuplicateCssDeclarationList(cssDeclarationList);
        UI_CssStyleSheetMergeRule(document->CssStyleSheet, rule);
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
    li = listGetIterator(cssStyleSheet->Rules, AL_START_HEAD);
    printf("\n");
    while (0 != (ln = listNext(li))) {
        rule = (uiCssRule_t*)listNodeValue(ln);

        liSelectorSection = listGetIterator(rule->Selector->Sections, AL_START_HEAD);
        while (0 != (lnSelectorSection = listNext(liSelectorSection))) {
            selectorSection = (uiCssSelectorSection_t*)listNodeValue(lnSelectorSection);

            printf("%s ", selectorSection->Value);

            if (UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE != selectorSection->AttributeType) {
                printf("%d %s", selectorSection->AttributeType, selectorSection->Attribute);
            }
        }
        listReleaseIterator(liSelectorSection);

        printf(" {");
        liCssDeclaration = listGetIterator(rule->CssDeclarationList->Data, AL_START_HEAD);
        while (0 != (lnCssDeclaration = listNext(liCssDeclaration))) {
            cssDeclaration = (uiCssDeclaration_t*)listNodeValue(lnCssDeclaration);

            printf("%d %s:%s; ", cssDeclaration->Type, cssDeclaration->Key, cssDeclaration->Value);
        }
        listReleaseIterator(liCssDeclaration);
        printf("} ");
        printf("\n");
    }
    listReleaseIterator(li);
}
