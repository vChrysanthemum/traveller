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
                            <td>   <a>hello</a>    world   ! </td>\
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
