#include <string.h>

#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

const char* UIERROR_CSS_PARSE_STATE_NOT_SELECTOR;
const char* UIERROR_CSS_PARSE_STATE_NOT_CSS_DECLARATION_KEY;
const char* UIERROR_CSS_PARSE_STATE_NOT_CSS_DECLARATION_VALUE;

void UI_PrepareDocument() {
    UI_PrepareHtml();
    UI_PrepareCss();
}

uiDocumentScanToken_t* UI_NewDocumentScanToken() {
    uiDocumentScanToken_t *token = (uiDocumentScanToken_t*)zmalloc(sizeof(uiDocumentScanToken_t));
    token->content = sdsempty();
    return token;
}

void UI_FreeDocumentScanToken(uiDocumentScanToken_t *token) {
    if (0 != token->content) sdsfree(token->content);
    zfree(token);
}

uiDocument_t* UI_NewDocument() {
    uiDocument_t *document = (uiDocument_t*)zmalloc(sizeof(uiDocument_t));
    memset(document, 0, sizeof(uiDocument_t));
    document->cssStyleSheet = UI_NewCssStyleSheet();
    document->script = sdsempty();
    document->style = sdsempty();
    return document;
}

void UI_FreeDocument(uiDocument_t* document) {
    if (0 != document->rootDom) UI_FreeHtmlDom(document->rootDom);
    UI_FreeCssStyleSheet(document->cssStyleSheet);
    if (0 != document->title) sdsfree(document->title);
    if (0 != document->script) sdsfree(document->script);
    if (0 != document->style) sdsfree(document->style);
}

uiDocument_t* UI_ParseDocument(char *documentContent) {
    uiDocument_t *document = UI_NewDocument();
    document->content = documentContent;
    UI_ParseHtml(document);
    UI_ParseCssStyleSheet(document, document->style);
    UI_ComputeHtmlDomTreeStyle(document);
    UI_RenderDocument(document);
    return document;
}
