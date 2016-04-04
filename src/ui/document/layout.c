#include <string.h>

#include "core/errors.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/extern.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

static inline void BreakLine(uiDocument_t *document, uiHtmlDom_t *dom) {
    document->layoutEnvironment.StartX = 0;
    document->layoutEnvironment.StartY += dom->Parent->layoutCurrentRowHeight + 1;
    dom->Parent->layoutChildrenHeight += dom->Parent->layoutCurrentRowHeight;
    dom->Parent->layoutCurrentRowHeight = 0;
}

static void LayoutHtmlDomStyleBlock(uiDocument_t *document, uiHtmlDom_t *dom) {
    if (dom->Parent->layoutChildrenWidth > 0) {
        BreakLine(document, dom);
    }

    //Assert document->layoutEnvironment.StartX == 0
    dom->Style.PositionStartX = document->layoutEnvironment.StartX;
    dom->Style.PositionStartY = document->layoutEnvironment.StartY;
    dom->Style.Width = dom->Parent->Style.Width;

    dom->Parent->layoutChildrenWidth = dom->Style.Width;
}

static void LayoutHtmlDomStyleInlineBlock(uiDocument_t *document, uiHtmlDom_t *dom) {
    if (1 == dom->Style.IsWidthPercent) {
        dom->Style.Width = dom->Parent->Style.Width * 0.01 * dom->Style.WidthPercent;
    }

    if (UIHTML_DOM_TYPE_TD == dom->Info->Type) {
        if (dom->Style.Width + dom->Parent->layoutChildrenWidth > dom->Parent->Style.Width) {
            dom->Style.Display = HTML_CSS_STYLE_DISPLAY_NONE;
        }
    } else {
        if (dom->Style.Width + dom->Parent->layoutChildrenWidth > dom->Parent->Style.Width) {
            BreakLine(document, dom);
        }

        dom->Style.PositionStartX = document->layoutEnvironment.StartX;
        dom->Style.PositionStartY = document->layoutEnvironment.StartY;
    }
}

static void ComputeHtmlDomWidth(uiDocument_t *document, uiHtmlDom_t *dom) {
    switch (dom->Style.Display) {
        case HTML_CSS_STYLE_DISPLAY_NONE:
            return;
            break;
        case HTML_CSS_STYLE_DISPLAY_BLOCK:
            LayoutHtmlDomStyleBlock(document, dom);
            break;
        case HTML_CSS_STYLE_DISPLAY_INLINE_BLOCK:
            LayoutHtmlDomStyleInlineBlock(document, dom);
            break;
    }
}

static void ComputeHtmlDomHeight(uiDocument_t *document, uiHtmlDom_t *dom) {
    if (UIHTML_DOM_TYPE_TEXT == dom->Info->Type) {
        dom->Style.Height = (dom->ContentUtf8Width / dom->Style.Width) + (0 == dom->ContentUtf8Width%dom->Style.Width ? 0 : 1);
    } else {
        dom->Style.Height = dom->layoutCurrentRowHeight + dom->layoutChildrenHeight;
    }

    if (0 != dom->Parent && dom->Style.Height > dom->Parent->layoutCurrentRowHeight) {
        dom->Parent->layoutCurrentRowHeight = dom->Style.Height;
    }

}

static void LayoutHtmlDomTree(uiDocument_t *document, uiHtmlDom_t *dom) {
    dom->layoutChildrenWidth = 0;
    dom->layoutCurrentRowHeight = 0;
    dom->layoutChildrenHeight = 0;

    if (0 == dom->Parent) {
        dom->Style.Width = document->layoutEnvironment.Width;
    } else {
        if (HTML_CSS_STYLE_DISPLAY_NONE == dom->Style.Display) {
            return;
        }

        ComputeHtmlDomWidth(document, dom);

        if (HTML_CSS_STYLE_DISPLAY_NONE == dom->Style.Display) {
            return;
        }
    }

    uiHtmlDom_t *childDom;
    listIter *li = listGetIterator(dom->Children, AL_START_HEAD); 
    listNode *ln;
    while (0 != (ln = listNext(li))) {
        childDom = (uiHtmlDom_t*)listNodeValue(ln);
        LayoutHtmlDomTree(document, childDom);
    }
    listReleaseIterator(li);

    ComputeHtmlDomHeight(document, dom);
}

void UI_LayoutDocument(uiDocument_t *document, int winWidth) {
    document->layoutEnvironment.StartX = 0;
    document->layoutEnvironment.StartY = 0;
    document->layoutEnvironment.Width = winWidth;
    LayoutHtmlDomTree(document, document->RootDom);
}
