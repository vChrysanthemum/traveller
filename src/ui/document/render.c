#include <string.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

uiDocumentRenderObject_t *UI_newDocumentRenderObject(uiHtmlDom_t *dom) {
    uiDocumentRenderObject_t *renderObject = (uiDocumentRenderObject_t*)zmalloc(sizeof(uiDocumentRenderObject_t));
    memset(renderObject, 0, sizeof(uiDocumentRenderObject_t));
    renderObject->dom = dom;
    return renderObject;
}

void UI_freeDocumentRenderObject(uiDocumentRenderObject_t *renderObject) {
    zfree(renderObject->textAlign);
    zfree(renderObject);
}
