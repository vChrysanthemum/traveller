#include "ui/ui.h"

uiDocumentScanToken_t* UI_NewDocumentScanToken() {
    uiDocumentScanToken_t *token = (uiDocumentScanToken_t*)zmalloc(sizeof(uiDocumentScanToken_t));
    token->content = sdsempty();
    return token;
}

void UI_FreeDocumentScanToken(uiDocumentScanToken_t *token) {
    sdsfree(token->content);
    zfree(token);
}

uiDocument_t* UI_NewDocument() {
    uiDocument_t *document = (uiDocument_t*)zmalloc(sizeof(uiDocument_t));
    memset(document, 0, sizeof(uiDocument_t));
    return document;
}

uiDocument_t* UI_ParseDocument(char *documentContent) {
    uiDocument_t *document = UI_NewDocument();
    document->content = documentContent;
    UI_ParseHtml(document);
    return document;
}
