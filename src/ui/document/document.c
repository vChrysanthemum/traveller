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
