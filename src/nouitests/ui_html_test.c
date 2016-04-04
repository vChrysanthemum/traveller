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

TEST_CASE("fail html token parser test")
{
    char *html = "\
                  <div>\
                  <input type=\"text\" name=\"text\" />\
                  hello world ! \
                  </div>\
                  ";

    uiDocumentScanner_t htmlScanner = {
        0, html, html, UI_ScanHtmlToken
    };

    uiDocumentScanToken_t *token = htmlScanner.Scan(&htmlScanner);
	REQUIRE(token, "should pass");

	REQUIRE_EQ(token->Type, UIHTML_TOKEN_START_TAG, "err: %s", token->Content);
    REQUIRE_EQ(0, strcmp("<div>", token->Content), "err: %s", token->Content);
    //freeHtmlToken

    token = htmlScanner.Scan(&htmlScanner);
	REQUIRE_EQ(token->Type, UIHTML_TOKEN_SELF_CLOSING_TAG, "err: %s", token->Content);
    REQUIRE_EQ(0, strcmp("<input type=\"text\" name=\"text\" />", token->Content), "err: %s", token->Content);

    token = htmlScanner.Scan(&htmlScanner);
	REQUIRE_EQ(token->Type, UIHTML_TOKEN_TEXT, "err: %s", token->Content);
    REQUIRE_EQ(0, strcmp("hello world !", token->Content), "err: %s", token->Content);

    token = htmlScanner.Scan(&htmlScanner);
	REQUIRE_EQ(token->Type, UIHTML_TOKEN_END_TAG, "err: %s", token->Content);
    REQUIRE_EQ(0, strcmp("</div>", token->Content), "err: %s", token->Content);

    token = htmlScanner.Scan(&htmlScanner);
    REQUIRE(0 == token, "err");
}

TEST_CASE("fail html parser test")
{
    listNode *ln;
    uiHtmlDom_t *dom;
    char *html = "\
                  <div id=\"top\" style=\"background:black\" data=\"shit '' \\\" shit\" class=\"top hover\">\
                    <input type=\"text\" name=\"text\" />\
                    <table>\
                        <tr>\
                            <td>   hello    world   ! </td>\
                        </tr> \
                    </table>\
                    <script>\
                    hello sdlkfjsdfonounoi123oi12n3oin \
                    </script>\
                    hello  &gt; &nbsp; sdlkfj \
                  </div>\
                  <style>\
                  .hello {} \
                  div {} \
                  </style>\
                  ";
    uiDocument_t *document = UI_NewDocument();
    document->Content = html;
    UI_ParseHtml(document);

    uiHtmlDom_t *rootDom = document->RootDom;
    UI_PrintHtmlDomTree(rootDom, 0);

    ln = rootDom->Children->head;
    uiHtmlDom_t *rootDivDom = listNodeValue(ln);

    REQUIRE_EQ(4, listLength(rootDivDom->Children), "err");

    REQUIRE_EQ(0, strcmp("div", rootDivDom->Title), "err");

    REQUIRE_EQ(2, listLength(rootDivDom->Classes), "err");
    REQUIRE_EQ(0, strcmp("top", rootDivDom->Id), "err");
    REQUIRE_EQ(0, strcmp("top", (sds)listNodeValue(rootDivDom->Classes->head)), "err");
    REQUIRE_EQ(0, strcmp("hover", (sds)listNodeValue(rootDivDom->Classes->head->next)), "err");

    ln = rootDivDom->Children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("input", dom->Title), "err");

    ln = ln->next;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("table", dom->Title), "err");

    ln = dom->Children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("tr", dom->Title), "err");

    ln = dom->Children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("td", dom->Title), "err");

    ln = dom->Children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("hello world !", dom->Content), "err");
}
