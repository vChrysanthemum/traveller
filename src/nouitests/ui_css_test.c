#include <string.h>

#include "case.h"
#include "core/util.h"
#include "ui/ui.h"

TEST_CASE("fail css parser test")
{
    char *html = "\
                  <style>\
                  body { background-color:black; } \
                  div div.test { width:30px; }\
                  #head { width:30px; }\
                  #head div { width:20px; }\
                  #head div div div a input { width:20px; }\
                  </style>\
                  ";
    uiDocument_t *document = UI_ParseDocument(html);
    UI_PrintCssStyleSheet(document->cssStyleSheet);
}
