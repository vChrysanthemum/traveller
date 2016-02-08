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
    UIHTMLToken *token = UIHTMLNextToken(&ptr);
	REQUIRE(token, "should pass");
	REQUIRE_EQ(token->type, UIHTML_TOKEN_START_TAG, "<text>");
    REQUIRE_EQ(0, strcmp("<div>", token->content), "<text> value");
    //freeHtmlToken

    token = UIHTMLNextToken(&ptr);
	REQUIRE_EQ(token->type, UIHTML_TOKEN_SELF_CLOSING_TAG, "text");
    REQUIRE_EQ(0, strcmp("<input type=\"text\" name=\"text\" />", token->content), "text value");
}
