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
    zfree(renderObject);
}

static void RenderHtmlDom(uiDocument_t *document, uiHtmlDom_t *dom) {
    if (0 != dom->info->render) {
        dom->info->render(document, dom);
    }
}

static void RenderHtmlDomTree(uiDocument_t *document, uiHtmlDom_t *dom) {
    RenderHtmlDom(document, dom);

    listIter *li;
    listNode *ln;
    li = listGetIterator(dom->children, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        RenderHtmlDomTree(document, (uiHtmlDom_t*)listNodeValue(ln));
    }
    listReleaseIterator(li);
}

void UI_RenderDocument(uiDocument_t *document) {
    RenderHtmlDomTree(document, document->rootDom);
}
