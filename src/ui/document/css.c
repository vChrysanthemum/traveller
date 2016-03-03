#include "core/zmalloc.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/util.h"

#include "event/event.h"
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
    {"width",            UICSS_PROPERTY_TYPE_WIDTH},
    {"height",           UICSS_PROPERTY_TYPE_HEIGHT},
    {0},
};

static inline void skipStringNotConcern(char **ptr)  {
    while(UI_IsWhiteSpace(**ptr) && 0 != **ptr) {
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

uiCssPropertyList_t* UI_DuplicateCssPropertyList(uiCssPropertyList_t* propertyList) {
    propertyList->referenceCount++;
    return propertyList;
}

uiCssPropertyList_t* UI_NewCssPropertyList() {
    uiCssPropertyList_t *propertyList = (uiCssPropertyList_t*)zmalloc(sizeof(uiCssPropertyList_t));
    propertyList->referenceCount = 1;
    propertyList->data = listCreate();
    propertyList->data->free = UI_FreeCssProperty;
    return propertyList;
}

void UI_FreeCssPropertyList(uiCssPropertyList_t *propertyList) {
    propertyList--;
    if (propertyList < 0) {
        listRelease(propertyList->data);
        zfree(propertyList);
    }
}

uiCssSelectorSection_t* UI_NewCssSelectorSection() {
    uiCssSelectorSection_t *section = (uiCssSelectorSection_t*)zmalloc(sizeof(uiCssSelectorSection_t));
    memset(section, 0, sizeof(uiCssSelectorSection_t));
    section->type = UICSS_SELECTOR_SECTION_TYPE_UNKNOWN;
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
    UI_FreeCssPropertyList(rule->propertyList);
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
        uiCssSelector_t **selector, uiCssProperty_t **property) {
    switch(scanner->state) {
        case UICSS_PARSE_STATE_SELECTOR:
            if (0 == *selector) {
                *selector = UI_NewCssSelector();
            }

            uiCssSelectorSection_t *section = UI_NewCssSelectorSection();

            int isSelectorSectionAttributeFound = 0;
            int selectorSectionAttributeIndex = 1;
            section->attributeType = UICSS_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE;
            while(1) {
                if ('\0' == token->content[selectorSectionAttributeIndex]) {
                    break;
                }

                if ('.' == token->content[selectorSectionAttributeIndex]) {
                    section->attributeType = UICSS_SELECTOR_SECTION_ATTRIBUTE_TYPE_CLASS;
                    selectorSectionAttributeIndex++;
                    section->attribute = sdsnewlen(&token->content[selectorSectionAttributeIndex], 
                            sdslen(token->content)-selectorSectionAttributeIndex);
                    isSelectorSectionAttributeFound = 1;
                    break;
                }

                selectorSectionAttributeIndex++;
            }

            int selectorSectionValueIndex = 0;
            if('#' == *token->content) {
                section->type = UICSS_SELECTOR_SECTION_TYPE_ID;
                selectorSectionValueIndex = 1;
            } else if ('.' == *token->content) {
                section->type = UICSS_SELECTOR_SECTION_TYPE_CLASS;
                selectorSectionValueIndex = 1;
            } else {
                section->type = UICSS_SELECTOR_SECTION_TYPE_TAG;
            }

            if (1 == isSelectorSectionAttributeFound) {
                section->value = sdsnewlen(&token->content[selectorSectionValueIndex],
                        selectorSectionAttributeIndex-selectorSectionValueIndex-1);
            } else {
                section->value = sdsnewlen(&token->content[selectorSectionValueIndex],
                        sdslen(token->content)-selectorSectionValueIndex);
            }

            (*selector)->sections = listAddNodeTail((*selector)->sections, section);
            break;

        case UICSS_PARSE_STATE_PROPERTY_KEY:
            *property = UI_NewCssProperty();
            (*property)->key = sdsnewlen(token->content, sdslen(token->content));
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
    uiCssPropertyList_t* propertyList = UI_NewCssPropertyList();
    //properties->free = UI_FreeCssProperty;
    while(0 != (token = cssScanner.scan(&cssScanner))) {
        switch (token->type) {
            case UICSS_TOKEN_TEXT:
                parseCssTokenText(&cssScanner, token, &selector, &property);
                break;

            case UICSS_TOKEN_COMMA:
                AssertOrReturnError(UICSS_PARSE_STATE_SELECTOR == cssScanner.state,
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                if (0 != selector) {
                    selectors = listAddNodeTail(selectors, selector);
                    selector = 0;
                }
                break;

            case UICSS_TOKEN_BLOCK_START:
                AssertOrReturnError(UICSS_PARSE_STATE_SELECTOR == cssScanner.state,
                        UIERROR_CSS_PARSE_STATE_NOT_SELECTOR);

                if (0 != selector) {
                    selectors = listAddNodeTail(selectors, selector);
                    selector = 0;
                }
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

                if (0 != property) {
                    propertyList->data = listAddNodeTail(propertyList->data, property);
                    property = 0;
                }
                cssScanner.state = UICSS_PARSE_STATE_PROPERTY_KEY;
                break;

            case UICSS_TOKEN_BLOCK_END:
                if (0 != property) {
                    propertyList->data = listAddNodeTail(propertyList->data, property);
                    property = 0;
                }
                cssScanner.state = UICSS_PARSE_STATE_SELECTOR;
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
        rule->propertyList = UI_DuplicateCssPropertyList(propertyList);
        UI_CssStyleSheetMergeRule(document->cssStyleSheet, rule);
    }
    listReleaseIterator(li);

    listRelease(selectors);
    UI_FreeCssPropertyList(propertyList);

    return 0;
}


void UI_PrintCssStyleSheet(uiCssStyleSheet_t *cssStyleSheet) {
    uiCssRule_t *rule;
    uiCssSelectorSection_t *selectorSection;
    uiCssProperty_t *property;

    listIter *liSelectorSection;
    listNode *lnSelectorSection;

    listIter *liProperty;
    listNode *lnProperty;

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

            if (UICSS_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE != selectorSection->attributeType) {
                printf("%d %s", selectorSection->attributeType, selectorSection->attribute);
            }
        }
        listReleaseIterator(liSelectorSection);

        printf(" {");
        liProperty = listGetIterator(rule->propertyList->data, AL_START_HEAD);
        while (0 != (lnProperty = listNext(liProperty))) {
            property = (uiCssProperty_t*)listNodeValue(lnProperty);

            printf("%d %s:%s; ", property->type, property->key, property->value);
        }
        listReleaseIterator(liProperty);
        printf("} ");
        printf("\n");
    }
    listReleaseIterator(li);
}
