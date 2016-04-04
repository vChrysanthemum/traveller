#include <string.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

static void RenderHtmlDom(uiDocument_t *document, uiHtmlDom_t *dom) {
    if (0 != dom->Info->Render) {
        dom->Info->Render(document, dom);
    }
}

static void RenderHtmlDomTree(uiDocument_t *document, uiHtmlDom_t *dom) {
    RenderHtmlDom(document, dom);

    listIter *li;
    listNode *ln;
    li = listGetIterator(dom->Children, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        RenderHtmlDomTree(document, (uiHtmlDom_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);
}

void UI_RenderDocument(uiDocument_t *document) {
    RenderHtmlDomTree(document, document->RootDom);
}
