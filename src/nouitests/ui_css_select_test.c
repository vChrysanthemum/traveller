#include <string.h>

#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/extern.h"

#include "case.h"
#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

TEST_CASE("fail css scan leafHtmlDoms test")
{
    char *html = "\
                  <div id=\"top\" style=\"shit '' \\\" shit\" class=\"top hover\">\
                    <input type=\"text\" name=\"text\" />\
                    <table>\
                        <tr>\
                            <td>   <a><shit>sdfasdf</shit></a>    world   ! </td>\
                        </tr> \
                    </table>\
                    <script>\
                    hello sdlkfjsdfonounoi123oi12n3oin \
                    </script>\
                    hello  &gt; &nbsp; <img src="" /><div><a>.</a></div>sdlkfj \
                  </div>\
                  <style>\
                  .hello {} \
                  div {} \
                  </style>\
                  ";
    uiDocument_t *document = UI_ParseDocument(html);

    list* leafHtmlDoms = UI_ScanLeafHtmlDoms(document->RootDom);
    uiHtmlDom_t *dom;

    dom = listNodeValue(listIndex(leafHtmlDoms, 0));
    REQUIRE(0 == stringcmp("input", dom->Title), "err");

    dom = listNodeValue(listIndex(leafHtmlDoms, 1));
    REQUIRE(0 == stringcmp("shit", dom->Title), "err");

    dom = listNodeValue(listIndex(leafHtmlDoms, 2));
    REQUIRE(0 == stringcmp("img", dom->Title), "err");

    dom = listNodeValue(listIndex(leafHtmlDoms, 3));
    REQUIRE(0 == stringcmp("a", dom->Title), "err");
}

TEST_CASE("fail get dom by css selector test")
{
    char *html = "\
                  <div id=\"top\" style=\"shit '' \\\" shit\" class=\"top hover\">\
                    <input class=\"hello\" type=\"text\" name=\"text\" />\
                    <table>\
                        <tr class=\"hello\">\
                            <td>   <a class=\"hello\"><shit>sdfasdf</shit></a>    world   ! </td>\
                        </tr> \
                    </table>\
                    <table>\
                        <shit class=\"hello\" />\
                    </table>\
                    <script>\
                    hello sdlkfjsdfonounoi123oi12n3oin \
                    </script>\
                    hello  &gt; &nbsp; <img src="" /><div><a>.</a></div>sdlkfj \
                  </div>\
                  <style>\
                  table .hello {} \
                  </style>\
                  ";
    uiDocument_t *document = UI_ParseDocument(html);
    uiCssRule_t *rule = (uiCssRule_t*)listNodeValue(document->CssStyleSheet->Rules->head);
    list *doms = UI_GetHtmlDomsByCssSelector(document, rule->Selector);

    uiHtmlDom_t *dom;
    listNode *lndom;
    listIter *lidom;
    lidom = listGetIterator(doms, AL_START_HEAD);

    lndom = listNext(lidom);
    dom = (uiHtmlDom_t*)listNodeValue(lndom);
    REQUIRE(0 == stringcmp(dom->Title, "tr"), "err");

    lndom = listNext(lidom);
    dom = (uiHtmlDom_t*)listNodeValue(lndom);
    REQUIRE(0 == stringcmp(dom->Title, "a"), "err");

    lndom = listNext(lidom);
    dom = (uiHtmlDom_t*)listNodeValue(lndom);
    REQUIRE(0 == stringcmp(dom->Title, "shit"), "err");

    listReleaseIterator(lidom);
}
