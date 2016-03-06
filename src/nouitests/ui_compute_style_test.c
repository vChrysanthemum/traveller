#include <string.h>

#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"

#include "case.h"
#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

TEST_CASE("fail compute style test")
{
    char *html = "\
                  <html>\
                  <head>\
                    <title>\
                    </title>\
                  </head>\
                  <body>\
                    <div class=\"hello\">\
                        <table>\
                            <tr>\
                                <td class=\"active\"></td>\
                                <td></td>\
                                <td></td>\
                                <td class=\"active\"></td>\
                            </tr>\
                        </table>\
                    </div>\
                  </body>\
                  </html>\
                  <style>\
                  body { background-color:black; } \
                  .hello table td { padding-top:10;padding-bottom:10;padding-left:10; }\
                  .hello table td.active { background:black;padding-left:10; }\
                  </style>\
                  ";
    uiDocument_t *document = UI_ParseDocument(html);
    uiCssSelector_t *selector = 0;
    UI_CompileCssSelector(&selector, "div.hello table td.active");

    uiCssSelectorSection_t *selectorSection;

    selectorSection = listNodeValue(listIndex(selector->sections, 0));
    REQUIRE(0 == stringcmp(selectorSection->value, "div"), "err");
    REQUIRE(0 == stringcmp(selectorSection->attribute, "hello"), "err");

    selectorSection = listNodeValue(listIndex(selector->sections, 1));
    REQUIRE(0 == stringcmp(selectorSection->value, "table"), "err");

    selectorSection = listNodeValue(listIndex(selector->sections, 2));
    REQUIRE(0 == stringcmp(selectorSection->value, "td"), "err");
    REQUIRE(0 == stringcmp(selectorSection->attribute, "active"), "err");

    list *doms = UI_GetHtmlDomsByCssSelector(document, selector);

    uiHtmlDom_t *dom;
    listIter *liDom;
    listNode *lnDom;
    liDom = listGetIterator(doms, AL_START_HEAD);
    while (0 != (lnDom = listNext(liDom))) {
        dom = (uiHtmlDom_t*)listNodeValue(lnDom);
        REQUIRE(10 == dom->renderObject->paddingTop, "err");
        REQUIRE(10 == dom->renderObject->paddingLeft, "err");
    }
    listReleaseIterator(liDom);

    REQUIRE(2 == listLength(doms), "err");
}
