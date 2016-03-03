#include <string.h>

#include "core/zmalloc.h"
#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"

#include "case.h"
#include "event/event.h"
#include "ui/ui.h"

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

    list* leafHtmlDoms = UI_ScanLeafHtmlDoms(document->rootDom);
    uiHtmlDom_t *dom;
    listIter *li;
    listNode *ln;
    li = listGetIterator(leafHtmlDoms, AL_START_HEAD);
    printf("\nleafHtmlDoms: ");
    while(0 != (ln = listNext(li))) {
        dom = (uiHtmlDom_t*)listNodeValue(ln);
        printf("%s ", dom->title);
    }
    printf("\n");
    listReleaseIterator(li);

    listRelease(leafHtmlDoms);
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
    uiCssRule_t *rule = (uiCssRule_t*)listNodeValue(document->cssStyleSheet->rules->head);
    list *doms = UI_GetHtmlDomsByCssSelector(document, rule->selector);

    uiHtmlDom_t *dom;
    listNode *lndom;
    listIter *lidom;
    lidom = listGetIterator(doms, AL_START_HEAD);

    lndom = listNext(lidom);
    dom = (uiHtmlDom_t*)listNodeValue(lndom);
    REQUIRE(0 == stringcmp(dom->title, "tr"), "err");

    lndom = listNext(lidom);
    dom = (uiHtmlDom_t*)listNodeValue(lndom);
    REQUIRE(0 == stringcmp(dom->title, "a"), "err");

    lndom = listNext(lidom);
    dom = (uiHtmlDom_t*)listNodeValue(lndom);
    REQUIRE(0 == stringcmp(dom->title, "shit"), "err");

    listReleaseIterator(lidom);
}
