#include <string.h>

#include "case.h"
#include "core/util.h"
#include "ui/ui.h"

TEST_CASE("fail html token parser test")
{
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
