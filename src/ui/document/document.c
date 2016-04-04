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
    token->Content = sdsempty();
    return token;
}

void UI_FreeDocumentScanToken(uiDocumentScanToken_t *token) {
    if (0 != token->Content) sdsfree(token->Content);
    zfree(token);
}

uiDocument_t* UI_NewDocument() {
    uiDocument_t *document = (uiDocument_t*)zmalloc(sizeof(uiDocument_t));
    memset(document, 0, sizeof(uiDocument_t));
    document->CssStyleSheet = UI_NewCssStyleSheet();
    document->Script = sdsempty();
    document->Style = sdsempty();
    document->layoutEnvironment.StartX = 0;
    document->layoutEnvironment.StartY = 0;
    return document;
}

void UI_FreeDocument(uiDocument_t* document) {
    if (0 != document->RootDom) UI_FreeHtmlDom(document->RootDom);
    UI_FreeCssStyleSheet(document->CssStyleSheet);
    if (0 != document->Title) sdsfree(document->Title);
    if (0 != document->Script) sdsfree(document->Script);
    if (0 != document->Style) sdsfree(document->Style);
}

uiDocument_t* UI_ParseDocumentWithoutRender(char *documentContent) {
    uiDocument_t *document = UI_NewDocument();
    document->Content = documentContent;
    UI_ParseHtml(document);
    UI_ParseCssStyleSheet(document, document->Style);
    UI_ComputeHtmlDomTreeStyle(document);
    return document;
}

uiDocument_t* UI_ParseDocument(char *documentContent) {
    uiDocument_t *document = UI_ParseDocumentWithoutRender(documentContent);
    UI_LayoutDocument(document);
    UI_RenderDocument(document);
    return document;
}
