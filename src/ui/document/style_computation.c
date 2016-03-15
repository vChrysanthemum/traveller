#include <stdlib.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

static inline unsigned int ConvertCssDeclarationValueToUnsignedInt(sds value) {
    int result = atoi(value);
    return result <= 0 ? 0 : (unsigned int)result;
}

static inline void ComputeHtmlDomStyleByCssDeclaration(uiDocument_t *document, uiHtmlDom_t *dom, uiCssDeclaration_t *cssDeclaration) {
    listIter *liCssDeclaration;
    listNode *lnCssDeclaration;
    uiCssDeclaration_t *_cssDeclaration;
    int isMatchFound = 0;
    liCssDeclaration = listGetIterator(dom->cssDeclarations, AL_START_HEAD);
    while (0 != (lnCssDeclaration = listNext(liCssDeclaration))) {
        _cssDeclaration = (uiCssDeclaration_t*)listNodeValue(lnCssDeclaration);
        if (cssDeclaration->type == _cssDeclaration->type) {
            UI_UpdateCssDeclaration(_cssDeclaration, cssDeclaration);
            isMatchFound = 1;
            break;
        }
    }
    listReleaseIterator(liCssDeclaration);
    if (0 == isMatchFound) {
        _cssDeclaration = UI_DuplicateCssDeclaration(cssDeclaration);
        dom->cssDeclarations = listAddNodeTail(dom->cssDeclarations, _cssDeclaration);
    }

    switch (cssDeclaration->type) {
        case UI_CSS_DECLARATION_TYPE_UNKNOWN:
            break;

        case UI_CSS_DECLARATION_TYPE_BACKGROUND_COLOR:
            dom->style.backgroundColor = UI_GetColorIntByColorString(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_COLOR:
            dom->style.color = UI_GetColorIntByColorString(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING:
            dom->style.paddingTop        =
                dom->style.paddingBottom =
                dom->style.paddingLeft   =
                dom->style.paddingRight  = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_TOP:
            dom->style.paddingTop = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_BOTTOM:
            dom->style.paddingBottom = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_LEFT:
            dom->style.paddingLeft = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_RIGHT:
            dom->style.paddingRight = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN:
            dom->style.marginTop        =
                dom->style.marginBottom =
                dom->style.marginLeft   =
                dom->style.marginRight  = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_TOP:
            dom->style.marginTop = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_BOTTOM:
            dom->style.marginBottom = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_LEFT:
            dom->style.marginLeft = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_RIGHT:
            dom->style.marginRight = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_DISPLAY:
            if (0 == stringcmp("none", cssDeclaration->value)) {
                dom->style.isHide = 1;
            }
            break;

        case UI_CSS_DECLARATION_TYPE_TEXT_ALIGN:
            dom->style.textAlign = sdsupdate(dom->style.textAlign, cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_WIDTH:
            dom->style.width = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_HEIGHT:
            dom->style.height = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->value);
            break;

        case UI_CSS_DECLARATION_TYPE_POSITION:
            dom->style.textAlign = sdsupdate(dom->style.textAlign, cssDeclaration->value);
            break;
    }
}

static void ComputeHtmlDomTreeStyleByCssRule(uiDocument_t *document, uiCssRule_t *cssRule) {
    list *doms = UI_GetHtmlDomsByCssSelector(document, cssRule->selector);

    if (0 == doms) {
        return;
    }

    if (0 == cssRule->cssDeclarationList) {
        return;
    }

    listIter *liDom;
    listNode *lnDom;
    listIter *liCssDeclaration;
    listNode *lnCssDeclaration;
    liDom = listGetIterator(doms, AL_START_HEAD);
    while (0 != (lnDom = listNext(liDom))) {

        liCssDeclaration = listGetIterator(cssRule->cssDeclarationList->data, AL_START_HEAD);
        while (0 != (lnCssDeclaration = listNext(liCssDeclaration))) {
            ComputeHtmlDomStyleByCssDeclaration(document,
                    (uiHtmlDom_t*)listNodeValue(lnDom),
                    (uiCssDeclaration_t*)listNodeValue(lnCssDeclaration));
        }
        listReleaseIterator(liCssDeclaration);
    }
    listReleaseIterator(liDom);
}

static void ComputeHtmlDomStyleByDomAttributeStyle(uiDocument_t *document, uiHtmlDom_t *dom) {
    if (1 == UI_IsHtmlDomNotCareCssDeclaration(dom)) {
        return;
    }

    if (0 == dom->styleCssDeclarations || 0 == listLength(dom->styleCssDeclarations)) {
        return;
    }

    listIter *liCssDeclaration;
    listNode *lnCssDeclaration;
    liCssDeclaration = listGetIterator(dom->styleCssDeclarations, AL_START_HEAD);
    while (0 != (lnCssDeclaration = listNext(liCssDeclaration))) {
        ComputeHtmlDomStyleByCssDeclaration(document, dom, (uiCssDeclaration_t*)listNodeValue(lnCssDeclaration));
    }
    listReleaseIterator(liCssDeclaration);

    return;
}

static void ComputeHtmlDomTreeStyleByDomAttributeStyle(uiDocument_t *document, uiHtmlDom_t *dom) {
    ComputeHtmlDomStyleByDomAttributeStyle(document, dom);

    listIter *liDom;
    listNode *lnDom;
    liDom = listGetIterator(dom->children, AL_START_HEAD);
    while (0 != (lnDom = listNext(liDom))) {
        ComputeHtmlDomTreeStyleByDomAttributeStyle(document, (uiHtmlDom_t*)listNodeValue(lnDom));
    }
    listReleaseIterator(liDom);
}

void UI_ComputeHtmlDomTreeStyle(uiDocument_t *document) {
    listIter *liCssRule;
    listNode *lnCssRule;
    liCssRule = listGetIterator(document->cssStyleSheet->rules, AL_START_HEAD);
    while (0 != (lnCssRule = listNext(liCssRule))) {
        ComputeHtmlDomTreeStyleByCssRule(document, (uiCssRule_t*)listNodeValue(lnCssRule));
    }
    listReleaseIterator(liCssRule);

    ComputeHtmlDomTreeStyleByDomAttributeStyle(document, document->rootDom);
}
