#include <string.h>

#include "case.h"
#include "core/util.h"
#include "ui/ui.h"

TEST_CASE("fail html token parser test")
{
    UIHtmlPrepare();
    char *html = "\
                  <div>\
                  <input type=\"text\" name=\"text\" />\
                  hello world ! \
                  </div>\
                  ";

    char *ptr = html;
    UIHtmlToken *token = UIHtmlNextToken(&ptr);
	REQUIRE(token, "should pass");

	REQUIRE_EQ(token->type, UIHTML_TOKEN_START_TAG, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("<div>", token->content), "err: %s", token->content);
    //freeHtmlToken

    token = UIHtmlNextToken(&ptr);
	REQUIRE_EQ(token->type, UIHTML_TOKEN_SELF_CLOSING_TAG, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("<input type=\"text\" name=\"text\" />", token->content), "err: %s", token->content);

    token = UIHtmlNextToken(&ptr);
	REQUIRE_EQ(token->type, UIHTML_TOKEN_TEXT, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("hello world !", token->content), "err: %s", token->content);

    token = UIHtmlNextToken(&ptr);
	REQUIRE_EQ(token->type, UIHTML_TOKEN_END_TAG, "err: %s", token->content);
    REQUIRE_EQ(0, strcmp("</div>", token->content), "err: %s", token->content);

    token = UIHtmlNextToken(&ptr);
    REQUIRE(0 == token, "err");
}

TEST_CASE("fail html parser test")
{
    UIHtmlPrepare();
    listNode *ln;
    UIHtmlDom *dom;
    char *html = "\
                  <div style=\"shit '' \\\" shit\">\
                    <input type=\"text\" name=\"text\" />\
                    <table>\
                        <tr>\
                            <td>   hello    world   ! </td>\
                        </tr> \
                    </table>\
                    <script>\
                    hello sdlkfjsdfonounoi123oi12n3oin \
                    hoaisd aslkdfj\
                    </script>\
                    hello sdlkfj \
                  </div>\
                  ";
    UIHtmlDom *rootDom = UIParseHtml(html);
    UIHtmlPrintDomTree(rootDom, 0);

    ln = rootDom->children->head;
    UIHtmlDom *rootDivDom = listNodeValue(ln);

    REQUIRE_EQ(4, listLength(rootDivDom->children), "err");

    REQUIRE_EQ(0, strcmp("div", rootDivDom->title), "err");

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
