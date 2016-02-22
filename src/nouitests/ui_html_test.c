#include <string.h>

#include "case.h"
#include "core/util.h"
#include "ui/ui.h"

TEST_CASE("fail html token parser test")
{
    UI_PrepareDocument();
    char *html = "\
                  <div>\
                  <input type=\"text\" name=\"text\" />\
                  hello world ! \
                  </div>\
                  ";

    uiDocumentScanner_t htmlScanner = {
        0, html, html, UI_ScanHtmlToken
    };

    uiDocumentScanToken_t *token = htmlScanner.scan(&htmlScanner);
	REQUIRE(token, "should pass");

	REQUIRE_EQ(token->type, UIHTML_TOKEN_START_TAG, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("<div>", token->content), "err: %s", token->content);
    //freeHtmlToken

    token = htmlScanner.scan(&htmlScanner);
	REQUIRE_EQ(token->type, UIHTML_TOKEN_SELF_CLOSING_TAG, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("<input type=\"text\" name=\"text\" />", token->content), "err: %s", token->content);

    token = htmlScanner.scan(&htmlScanner);
	REQUIRE_EQ(token->type, UIHTML_TOKEN_TEXT, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("hello world !", token->content), "err: %s", token->content);

    token = htmlScanner.scan(&htmlScanner);
	REQUIRE_EQ(token->type, UIHTML_TOKEN_END_TAG, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("</div>", token->content), "err: %s", token->content);

    token = htmlScanner.scan(&htmlScanner);
    REQUIRE(0 == token, "err");
}

TEST_CASE("fail html parser test")
{
    UI_PrepareDocument();
    listNode *ln;
    uiHtmlDom_t *dom;
    char *html = "\
                  <div id=\"top\" style=\"shit '' \\\" shit\" class=\"top hover\">\
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
    document->content = html;
    UI_ParseHtml(document);

    uiHtmlDom_t *rootDom = document->rootDom;
    UI_PrintHtmlDomTree(rootDom, 0);

    ln = rootDom->children->head;
    uiHtmlDom_t *rootDivDom = listNodeValue(ln);

    REQUIRE_EQ(4, listLength(rootDivDom->children), "err");

    REQUIRE_EQ(0, strcmp("div", rootDivDom->title), "err");

    REQUIRE_EQ(2, listLength(rootDivDom->classes), "err");
    REQUIRE_EQ(0, strcmp("top", rootDivDom->id), "err");
    REQUIRE_EQ(0, strcmp("top", (sds)listNodeValue(rootDivDom->classes->head)), "err");
    REQUIRE_EQ(0, strcmp("hover", (sds)listNodeValue(rootDivDom->classes->head->next)), "err");

    ln = rootDivDom->children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("input", dom->title), "err");

    ln = ln->next;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("table", dom->title), "err");

    ln = dom->children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("tr", dom->title), "err");

    ln = dom->children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("td", dom->title), "err");

    ln = dom->children->head;
    dom = listNodeValue(ln);
    REQUIRE_EQ(0, strcmp("hello world !", dom->content), "err");
}
