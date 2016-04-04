#include <string.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/extern.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

void UI_RenderHtmlDomBody(uiDocument_t *document, uiHtmlDom_t *dom) {
}

void UI_RenderHtmlDomTitle(uiDocument_t *document, uiHtmlDom_t *dom) {
    uiHtmlDom_t *textDom = (uiHtmlDom_t*)listNodeValue(dom->Children->head);
    if (0 == textDom) {
        return;
    }

    if (0 == document->Title) {
        document->Title = sdsnewlen(textDom->Content, sdslen(textDom->Content));
    } else {
        sdsclear(document->Title);
        document->Title = sdscatlen(document->Title, textDom->Content, sdslen(textDom->Content));
    }
}
