#include <string.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

static void LayoutHtmlDomTree(uiDocument_t *document, uiHtmlDom_t *dom) {
    if (0 == dom->Info || UIHTML_DOM_TYPE_UNDEFINED == dom->Info->Type) {
        return;
    }

    if (1 == dom->Style.IsHide) {
        return;
    }
}

void UI_LayoutDocument(uiDocument_t *document) {
    LayoutHtmlDomTree(document, document->RootDom);
}
