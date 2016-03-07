#include <string.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

// css选择器

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

        if (1 == UI_IsHtmlDomNotCareCssDeclaration(child)) {
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

static inline int IsSelectorSectionMatchHtmlDom(uiCssSelectorSection_t *selectorSection, uiHtmlDom_t *dom) {
    if (1 == UI_IsHtmlDomNotCareCssDeclaration(dom)) {
        return 0;
    }

    int result = 0;

    switch (selectorSection->type) {
        case UI_SELECTOR_SECTION_TYPE_UNKNOWN:
            break;

        case UI_SELECTOR_SECTION_TYPE_TAG:
            if (0 == stringcmp(dom->title, selectorSection->value)) {
                result = 1;
            }
            break;

        case UI_SELECTOR_SECTION_TYPE_CLASS:
            if (0 == dom->classes) {
                break;
            }

            listIter *liClass;
            listNode *lnClass;
            liClass = listGetIterator(dom->classes, AL_START_HEAD);
            while (0 != (lnClass = listNext(liClass))) {
                if (0 == stringcmp((char*)listNodeValue(lnClass), selectorSection->value)) {
                    result = 1;
                    break;
                }
            }
            listReleaseIterator(liClass);
            break;

        case UI_SELECTOR_SECTION_TYPE_ID:
            if (0 == dom->id) {
                break;
            }

            if (0 == stringcmp(dom->id, selectorSection->value)) {
                result = 1;
            }
            break;
    }

    if (0 == result) {
        return 0;
    }

    result = 0;

    switch (selectorSection->attributeType) {
        case  UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_NONE:
            result = 1;
            break;

        case  UI_SELECTOR_SECTION_ATTRIBUTE_TYPE_CLASS:
            if (0 == dom->classes) {
                break;
            }

            listIter *liClass;
            listNode *lnClass;
            liClass = listGetIterator(dom->classes, AL_START_HEAD);
            while (0 != (lnClass = listNext(liClass))) {
                if (0 == stringcmp((char*)listNodeValue(lnClass), selectorSection->attribute)) {
                    result = 1;
                    break;
                }
            }
            listReleaseIterator(liClass);
            break;
    }

    return result;
}

int UI_IsSelectorSectionsLeftMatchHtmlDoms(listIter *liSelectorSection, uiHtmlDom_t *dom) {
    listNode *lnSelectorSection = listNext(liSelectorSection);
    if (0 == lnSelectorSection) {
        return 1;
    }
    uiCssSelectorSection_t *selectorSection = (uiCssSelectorSection_t*)listNodeValue(lnSelectorSection);

    while (1) {
        // 如果该 selectorSection 匹配 该 dom，则继续下一轮匹配
        if (1 == IsSelectorSectionMatchHtmlDom(selectorSection, dom)) {
            break;
        }

        // 如果不匹配，则继续尝试下一个dom
        dom = dom->parent;

        // dom 为 rootDom，则匹配失败
        if (0 == dom->parent) {
            return 0;
        }
    }

    return UI_IsSelectorSectionsLeftMatchHtmlDoms(liSelectorSection, dom);
}

// 根据一个 SelectorSection 选择多个 uiHtmlDom_t
// return doms
static list* SelectorSectionSelectHtmlDoms(uiCssSelectorSection_t *selectorSection, uiHtmlDom_t *dom) {
    if (0 == dom) {
        return 0;
    }

    list *doms = listCreate();

    if (1 == IsSelectorSectionMatchHtmlDom(selectorSection, dom)) {
        doms = listAddNodeTail(doms, dom);
    }

    if (1 == UI_IsHtmlDomNotCareCssDeclaration(dom) && 0 == listLength(dom->children)) {
        return doms;
    }

    listIter *lidom;
    listNode *lndom;

    list *subDoms;
    listIter *liSubDom;
    listNode *lnSubDom;

    lidom = listGetIterator(dom->children, AL_START_HEAD);
    while (0 != (lndom = listNext(lidom))) {
        subDoms = SelectorSectionSelectHtmlDoms(selectorSection, (uiHtmlDom_t*)listNodeValue(lndom));
        liSubDom = listGetIterator(subDoms, AL_START_HEAD);
        while (0 != (lnSubDom = listNext(liSubDom))) {
            doms = listAddNodeTail(doms, (uiHtmlDom_t*)listNodeValue(lnSubDom));
        }
        listReleaseIterator(liSubDom);
    }
    listReleaseIterator(lidom);

    return doms;
}

// return doms
static list* SelectorSectionsRightFindHtmlDoms(listIter *liSelectorSection, uiHtmlDom_t *dom) {
    if (0 == dom || 1 == UI_IsHtmlDomNotCareCssDeclaration(dom)) {
        return 0;
    }

    uiCssSelectorSection_t *selectorSection = (uiCssSelectorSection_t*)listNodeValue(liSelectorSection->next);

    if (0 == selectorSection) {
        return 0;
    }

    if (0 == liSelectorSection->next->next) {
        return SelectorSectionSelectHtmlDoms(selectorSection, dom);
    }

    if (1 == IsSelectorSectionMatchHtmlDom(selectorSection, dom)) {
        listNext(liSelectorSection);
    }

    list *doms = listCreate();
    listIter *liSubDom;
    listNode *lnSubDom;
    list *grandDoms;
    listIter *liGrandDom;
    listNode *lnGrandDom;
    liSubDom = listGetIterator(dom->children, AL_START_HEAD);
    while (0 != (lnSubDom = listNext(liSubDom))) {
        grandDoms = SelectorSectionsRightFindHtmlDoms(liSelectorSection,
                (uiHtmlDom_t*)listNodeValue(lnSubDom));

        if (0 == grandDoms) {
            continue;
        }

        liGrandDom = listGetIterator(grandDoms, AL_START_HEAD);
        while (0 != (lnGrandDom = listNext(liGrandDom))) {
            doms = listAddNodeTail(doms, listNodeValue(lnGrandDom));
        }
        listReleaseIterator(liGrandDom);
    }
    listReleaseIterator(liSubDom);

    return doms;
}

// return doms
list* UI_GetHtmlDomsByCssSelector(uiDocument_t* document, uiCssSelector_t *selector) {
    listIter *liSelectorSection = listGetIterator(selector->sections, AL_START_HEAD);
    list *doms = SelectorSectionsRightFindHtmlDoms(liSelectorSection, document->rootDom);
    listReleaseIterator(liSelectorSection);
    return doms;
}
