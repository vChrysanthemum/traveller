#include "ui/ui.h"

UIDocumentScanToken* UIDocumentNewScanToken() {
    UIDocumentScanToken *token = (UIDocumentScanToken*)zmalloc(sizeof(UIDocumentScanToken));
    token->content = sdsempty();
    return token;
}

void UIDocumentFreeScanToken(UIDocumentScanToken *token) {
    sdsfree(token->content);
    zfree(token);
}
