#include "ui/ui.h"

// css选择器

static inline int IsHtmlDomCssSelectorIgnore(enum uiHtmlDomType_e type) {
    if (UIHTML_DOM_TYPE_TEXT == type ||
            UIHTML_DOM_TYPE_SCRIPT == type ||
            UIHTML_DOM_TYPE_STYLE == type) {
        return 1;
    } else {
        return 0;
    }
}

list* UI_ScanLeafHtmlDoms(uiHtmlDom_t *dom) {
    if (0 == listLength(dom->children)) {
        return 0;
    }

    list* retLeafHtmlDoms = listCreate();
    uiHtmlDom_t *child;
    list* leafHtmlDoms;

    listIter *li;
    listNode *ln;
    listIter *subLi;
    listNode *subLn;

    li = listGetIterator(dom->children, AL_START_HEAD);
    while (0 != (ln = listNext(li))) {
        child = (uiHtmlDom_t*)listNodeValue(ln);

        if (1 == IsHtmlDomCssSelectorIgnore(child->type)) {
            continue;
        }

        leafHtmlDoms = UI_ScanLeafHtmlDoms(child);
        if (0 == leafHtmlDoms) {
            retLeafHtmlDoms = listAddNodeTail(retLeafHtmlDoms, child);
            continue;
        }

        subLi = listGetIterator(leafHtmlDoms, AL_START_HEAD);
        while (0 != (subLn = listNext(subLi))) {
            retLeafHtmlDoms = listAddNodeTail(retLeafHtmlDoms, listNodeValue(subLn));
        }
        listReleaseIterator(subLi);
    }
    listReleaseIterator(li);

    if (0 == listLength(retLeafHtmlDoms)) {
        listRelease(retLeafHtmlDoms);
        return 0;

    } else {
        return retLeafHtmlDoms;
    }
}

uiHtmlDom_t* UI_GetHtmlDomByCssSelector(uiDocument_t* document, uiCssSelector_t *selector) {
    return 0;
}
