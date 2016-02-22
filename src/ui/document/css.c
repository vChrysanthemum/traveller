#include "ui/ui.h"

#include "core/extern.h"
#include "ui/extern.h"

static dict *uiCssPropertyInfoDict;
static uiCssPropertyInfo_t uiCssPropertyInfoTable[] = {
    {"background-color", UICSS_PROPERTY_TYPE_BACKGROUND_COLOR},
    {"color",            UICSS_PROPERTY_TYPE_COLOR},
    {"padding",          UICSS_PROPERTY_TYPE_PADDING},
    {"margin",           UICSS_PROPERTY_TYPE_MARGIN},
    {"display",          UICSS_PROPERTY_TYPE_DISPLAY},
    {"text-align",       UICSS_PROPERTY_TYPE_TEXT_ALIGN},
    {0},
};

static inline void skipStringNotConcern(char **ptr)  {
    while(UIIsWhiteSpace(**ptr) && 0 != **ptr) {
        (*ptr)++;
    }
}

void UI_PrepareCss() {
    uiCssPropertyInfoDict = dictCreate(&stackStringTableDictType, 0);
    for (uiCssPropertyInfo_t *domInfo = &uiCssPropertyInfoTable[0]; 0 != domInfo->name; domInfo++) {
        dictAdd(uiCssPropertyInfoDict, domInfo->name, domInfo);
    }
}

uiCssProperty_t* UI_NewCssProperty() {
    uiCssProperty_t *property = (uiCssProperty_t*)zmalloc(sizeof(uiCssProperty_t));
    memset(property, 0, sizeof(uiCssProperty_t));
    property->type = UICSS_PROPERTY_TYPE_UNKNOWN;
    return property;
}

void UI_FreeCssProperty(void *_property) {
    uiCssProperty_t *property = (uiCssProperty_t*)_property;
    zfree(property);
}

list* UI_DuplicateCssProperties(list *properties) {
    list *newProperties = listCreate();
    newProperties->free = UI_FreeCssProperty;

    listIter *li;
    listNode *ln;
    li = listGetIterator(properties, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        newProperties = listAddNodeTail(newProperties, listNodeValue(ln));
    }
    listReleaseIterator(li);

    return newProperties;
}

uiCssSelectorSection_t* UI_NewCssSelectorSection() {
    uiCssSelectorSection_t *section = (uiCssSelectorSection_t*)zmalloc(sizeof(uiCssSelectorSection_t));
    memset(section, 0, sizeof(uiCssSelectorSection_t));
    section->type = UICSS_SELECTOR_TYPE_UNKNOWN;
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
    rule->properties = listCreate();
    rule->properties->free = UI_FreeCssProperty;
    return rule;
}

void UI_FreeCssRule(void *_rule) {
    uiCssRule_t *rule = (uiCssRule_t*)_rule;
    UI_FreeCssSelector(rule->selector);
    listRelease(rule->properties);
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

    if (',' == *ptr) token->type = UICSS_TOKEN_COMMA;
    else if (':' == *ptr) token->type = UICSS_TOKEN_COLON;
    else if (';' == *ptr) token->type = UICSS_TOKEN_SEMICOLON;
    else if ('{' == *ptr) token->type = UICSS_TOKEN_BLOCK_START;
    else if ('}' == *ptr) token->type = UICSS_TOKEN_BLOCK_END;
    else {
        token->type = UICSS_TOKEN_TEXT;
        char *startPtr = ptr;
        for (; !UIIsWhiteSpace(*ptr) &&
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
        uiCssSelector_t **selector, uiCssProperty_t **property) {
    switch(scanner->state) {
        case UICSS_PARSE_STATE_SELECTOR:
            if (0 == *selector) {
                *selector = UI_NewCssSelector();
            }
            uiCssSelectorSection_t *section = UI_NewCssSelectorSection();
            if('#' == *token->content) {
                section->type = UICSS_SELECTOR_TYPE_ID;
                section->value = sdsnewlen(token->content+1, sdslen(token->content)-1);
            } else if ('.' == *token->content) {
                section->type = UICSS_SELECTOR_TYPE_CLASS;
                section->value = sdsnewlen(token->content+1, sdslen(token->content)-1);
            } else {
                section->type = UICSS_SELECTOR_TYPE_TAG;
                section->value = sdsnewlen(token->content, sdslen(token->content));
            }
            (*selector)->sections = listAddNodeTail((*selector)->sections, section);
            break;

        case UICSS_PARSE_STATE_PROPERTY_KEY:
            *property = UI_NewCssProperty();
            uiCssPropertyInfo_t *propertyInfo = dictFetchValue(uiCssPropertyInfoDict, token->content);
            if (0 != propertyInfo) {
                (*property)->type = propertyInfo->type;
            }
            break;

        case UICSS_PARSE_STATE_PROPERTY_VALUE:
            (*property)->value = sdsnewlen(token->content, sdslen(token->content));
            break;
    }
    return 0;
}

const char* UI_ParseCssStyleSheet(uiDocument_t *document, char *cssContent) {
    uiDocumentScanner_t cssScanner = {
        UICSS_PARSE_STATE_SELECTOR, cssContent, cssContent, UI_ScanCssToken
    };

    uiDocumentScanToken_t *token;
    uiCssSelector_t *selector = 0;
    uiCssProperty_t *property = 0;
    list *selectors = listCreate();
    list *properties = listCreate();
    properties->free = UI_FreeCssProperty;
    while(0 != (token = cssScanner.scan(&cssScanner))) {
        switch (token->type) {
            case UICSS_TOKEN_TEXT:
                parseCssTokenText(&cssScanner, token, &selector, &property);
                break;

            case UICSS_TOKEN_COMMA:
                AssertOrReturnError(UICSS_PARSE_STATE_SELECTOR == cssScanner.state,
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                selectors = listAddNodeTail(selectors, selector);
                selector = 0;
                break;

            case UICSS_TOKEN_BLOCK_START:
                AssertOrReturnError(UICSS_PARSE_STATE_SELECTOR == cssScanner.state,
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                selectors = listAddNodeTail(selectors, selector);
                selector = 0;
                cssScanner.state = UICSS_PARSE_STATE_PROPERTY_KEY;
                break;

            case UICSS_TOKEN_COLON:
                AssertOrReturnError(UICSS_PARSE_STATE_PROPERTY_KEY == cssScanner.state,
                        UIERROR_CSS_PARSE_STATE_NOT_PROPERTY_KEY);

                cssScanner.state = UICSS_PARSE_STATE_PROPERTY_VALUE;
                break;

            case UICSS_TOKEN_SEMICOLON:
                AssertOrReturnError(UICSS_PARSE_STATE_PROPERTY_VALUE == cssScanner.state,
                        UIERROR_CSS_PARSE_STATE_NOT_PROPERTY_VALUE);

                properties = listAddNodeTail(properties, property);
                property = 0;
                cssScanner.state = UICSS_PARSE_STATE_PROPERTY_KEY;
                break;

            case UICSS_TOKEN_BLOCK_END:
                properties = listAddNodeTail(properties, property);
                cssScanner.state = UICSS_PARSE_STATE_SELECTOR;
                break;
        }

        UI_FreeDocumentScanToken(token);
    }


    uiCssRule_t *rule;

    listIter *li;
    listNode *ln;
    li = listGetIterator(selectors, AL_START_HEAD);
    while (NULL != (ln = listNext(li))) {
        rule = UI_NewCssRule();
        rule->selector = listNodeValue(ln);
        rule->properties = UI_DuplicateCssProperties(properties);
        UI_CssStyleSheetMergeRule(document->cssStyleSheet, rule);
    }
    listReleaseIterator(li);

    listRelease(selectors);
    listRelease(properties);

    return 0;
}


void UI_PrintCssStyleSheet(uiCssStyleSheet_t *cssStyleSheet) {
    uiCssRule_t *rule;
    uiCssSelectorSection_t *selectorSection;

    listIter *liSelectorSection;
    listNode *lnSelectorSection;

    listIter *li;
    listNode *ln;
    li = listGetIterator(cssStyleSheet->rules, AL_START_HEAD);
    printf("\n");
    while (NULL != (ln = listNext(li))) {
        rule = (uiCssRule_t*)listNodeValue(ln);

        liSelectorSection = listGetIterator(rule->selector->sections, AL_START_HEAD);
        while (NULL != (lnSelectorSection = listNext(liSelectorSection))) {
            selectorSection = (uiCssSelectorSection_t*)listNodeValue(lnSelectorSection);

            printf("%s ", selectorSection->value);
        }
        printf("\n");
        listReleaseIterator(liSelectorSection);
    }
    listReleaseIterator(li);
}
