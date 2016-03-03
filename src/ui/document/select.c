#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"

#include "event/event.h"
#include "ui/ui.h"

// css选择器

static inline int IsHtmlDomCssSelectorIgnore(enum uiHtmlDomType_e type) {
    if (UIHTML_DOM_TYPE_TEXT == type ||
            UIHTML_DOM_TYPE_SCRIPT == type ||
            UIHTML_DOM_TYPE_STYLE == type) {
        return TRUE;
    } else {
        return FALSE;
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

        if (TRUE == IsHtmlDomCssSelectorIgnore(child->type)) {
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

static int IsSelectorSectionsLeftMatchHtmlDoms(listIter *liSelectorSection, uiHtmlDom_t *dom) {
    listNode *lnSelectorSection = listNext(liSelectorSection);
    if (0 == lnSelectorSection) {
        return TRUE;
    }
    uiCssSelectorSection_t *selectorSection = (uiCssSelectorSection_t*)listNodeValue(lnSelectorSection);

    listIter *liClass;
    listNode *lnClass;
    int isMatch;
    while(1) {
        isMatch = FALSE;
        switch(selectorSection->type) {
            case UICSS_SELECTOR_SECTION_TYPE_UNKNOWN:
                break;

            case UICSS_SELECTOR_SECTION_TYPE_TAG:
                if (0 == stringcmp(dom->title, selectorSection->value)) {
                    isMatch = TRUE;
                }
                break;

            case UICSS_SELECTOR_SECTION_TYPE_CLASS:
                if (0 == dom->classes) {
                    break;
                }

                liClass = listGetIterator(dom->classes, AL_START_HEAD);
                while(0 != (lnClass = listNext(liClass))) {
                    if (0 == stringcmp((char*)lnClass, selectorSection->value)) {
                        isMatch = TRUE;
                        break;
                    }
                }
                listReleaseIterator(liClass);
                break;

            case UICSS_SELECTOR_SECTION_TYPE_ID:
                if (0 == dom->id) {
                    break;
                }

                if (0 == stringcmp(dom->id, selectorSection->value)) {
                    isMatch = TRUE;
                }
                break;
        }

        // 如果该 selectorSection 匹配 该 dom，则继续下一轮匹配
        if (TRUE == isMatch) {
            break;
        }

        dom = dom->parent;

        // 如果不匹配，且dom 为 rootDom，则匹配失败
        if (0 == dom->parent) {
            return FALSE;
        }
    }

    return IsSelectorSectionsLeftMatchHtmlDoms(liSelectorSection, dom);
}

static uiHtmlDom_t* SelectorSectionsRightFindHtmlDom(listIter *liSelectorSection) {
    return 0;
}

uiHtmlDom_t* UI_GetHtmlDomByCssSelector(uiDocument_t* document, uiCssSelector_t *selector) {
    return 0;
}
