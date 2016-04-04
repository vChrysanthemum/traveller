#include <stdlib.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/extern.h"

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
    liCssDeclaration = listGetIterator(dom->CssDeclarations, AL_START_HEAD);
    while (0 != (lnCssDeclaration = listNext(liCssDeclaration))) {
        _cssDeclaration = (uiCssDeclaration_t*)listNodeValue(lnCssDeclaration);
        if (cssDeclaration->Type == _cssDeclaration->Type) {
            UI_UpdateCssDeclaration(_cssDeclaration, cssDeclaration);
            isMatchFound = 1;
            break;
        }
    }
    listReleaseIterator(liCssDeclaration);
    if (0 == isMatchFound) {
        _cssDeclaration = UI_DuplicateCssDeclaration(cssDeclaration);
        dom->CssDeclarations = listAddNodeTail(dom->CssDeclarations, _cssDeclaration);
    }

    switch (cssDeclaration->Type) {
        case UI_CSS_DECLARATION_TYPE_UNKNOWN:
            break;

        case UI_CSS_DECLARATION_TYPE_BACKGROUND_COLOR:
            dom->Style.BackgroundColor = UI_GetColorIntByColorString(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_COLOR:
            dom->Style.Color = UI_GetColorIntByColorString(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING:
            dom->Style.PaddingTop        =
                dom->Style.PaddingBottom =
                dom->Style.PaddingLeft   =
                dom->Style.PaddingRight  = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_TOP:
            dom->Style.PaddingTop = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_BOTTOM:
            dom->Style.PaddingBottom = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_LEFT:
            dom->Style.PaddingLeft = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_PADDING_RIGHT:
            dom->Style.PaddingRight = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN:
            dom->Style.MarginTop        =
                dom->Style.MarginBottom =
                dom->Style.MarginLeft   =
                dom->Style.MarginRight  = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_TOP:
            dom->Style.MarginTop = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_BOTTOM:
            dom->Style.MarginBottom = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_LEFT:
            dom->Style.MarginLeft = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_MARGIN_RIGHT:
            dom->Style.MarginRight = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_DISPLAY:
            if (0 == stringcmp("none", cssDeclaration->Value)) {
                dom->Style.Display = HTML_CSS_STYLE_DISPLAY_NONE;
            } else if (0 == stringcmp("block", cssDeclaration->Value)) {
                dom->Style.Display = HTML_CSS_STYLE_DISPLAY_BLOCK;
            } else if (0 == stringcmp("inline-block", cssDeclaration->Value)) {
                dom->Style.Display = HTML_CSS_STYLE_DISPLAY_INLINE_BLOCK;
            }
            break;

        case UI_CSS_DECLARATION_TYPE_TEXT_ALIGN:
            if (0 == stringcmp("left", cssDeclaration->Value)) {
                dom->Style.TextAlign = LEFT;
            } else if (0 == stringcmp("center", cssDeclaration->Value)) {
                dom->Style.TextAlign = CENTER;
            } else if (0 == stringcmp("right", cssDeclaration->Value)) {
                dom->Style.TextAlign = RIGHT;
            }
            break;

        case UI_CSS_DECLARATION_TYPE_WIDTH:
            if ('%' == cssDeclaration->Value[sdslen(cssDeclaration->Value)-1]) {
                dom->Style.IsWidthPercent = 1;
                sdsrange(cssDeclaration->Value, 0, -2);
                dom->Style.WidthPercent = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            } else {
                dom->Style.IsWidthPercent = 0;
                dom->Style.Width = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            }
            break;

        case UI_CSS_DECLARATION_TYPE_HEIGHT:
            dom->Style.Height = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_POSITION:
            if (0 == stringcmp("relative", cssDeclaration->Value)) {
                dom->Style.Position = HTML_CSS_STYLE_POSITION_RELATIVE;
            } else if (0 == stringcmp("absolute", cssDeclaration->Value)) {
                dom->Style.Position = HTML_CSS_STYLE_POSITION_ABSOLUTE;
            } else if (0 == stringcmp("fixed", cssDeclaration->Value)) {
                dom->Style.Position = HTML_CSS_STYLE_POSITION_FIXED;
            } else if (0 == stringcmp("static", cssDeclaration->Value)) {
                dom->Style.Position = HTML_CSS_STYLE_POSITION_STATIC;
            }
            break;

        case UI_CSS_DECLARATION_TYPE_LEFT:
            dom->Style.Left = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_RIGHT:
            dom->Style.Right = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_TOP:
            dom->Style.Top = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;

        case UI_CSS_DECLARATION_TYPE_BOTTOM:
            dom->Style.Bottom = ConvertCssDeclarationValueToUnsignedInt(cssDeclaration->Value);
            break;
    }
}

static void ComputeHtmlDomTreeStyleByCssRule(uiDocument_t *document, uiCssRule_t *cssRule) {
    list *doms = UI_GetHtmlDomsByCssSelector(document, cssRule->Selector);

    if (0 == doms) {
        return;
    }

    if (0 == cssRule->CssDeclarationList) {
        return;
    }

    listIter *liDom;
    listNode *lnDom;
    listIter *liCssDeclaration;
    listNode *lnCssDeclaration;
    liDom = listGetIterator(doms, AL_START_HEAD);
    while (0 != (lnDom = listNext(liDom))) {

        liCssDeclaration = listGetIterator(cssRule->CssDeclarationList->Data, AL_START_HEAD);
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

    if (0 == dom->StyleCssDeclarations || 0 == listLength(dom->StyleCssDeclarations)) {
        return;
    }

    listIter *liCssDeclaration;
    listNode *lnCssDeclaration;
    liCssDeclaration = listGetIterator(dom->StyleCssDeclarations, AL_START_HEAD);
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
    liDom = listGetIterator(dom->Children, AL_START_HEAD);
    while (0 != (lnDom = listNext(liDom))) {
        ComputeHtmlDomTreeStyleByDomAttributeStyle(document, (uiHtmlDom_t*)listNodeValue(lnDom));
    }
    listReleaseIterator(liDom);
}

void UI_ComputeHtmlDomTreeStyle(uiDocument_t *document) {
    listIter *liCssRule;
    listNode *lnCssRule;
    liCssRule = listGetIterator(document->CssStyleSheet->Rules, AL_START_HEAD);
    while (0 != (lnCssRule = listNext(liCssRule))) {
        ComputeHtmlDomTreeStyleByCssRule(document, (uiCssRule_t*)listNodeValue(lnCssRule));
    }
    listReleaseIterator(liCssRule);

    ComputeHtmlDomTreeStyleByDomAttributeStyle(document, document->RootDom);
}
