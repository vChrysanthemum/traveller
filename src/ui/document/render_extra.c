#include <string.h>

#include "core/adlist.h"
#include "core/dict.h"
#include "core/sds.h"
#include "core/zmalloc.h"
#include "core/util.h"
#include "core/extern.h"

#include "event/event.h"
#include "ui/ui.h"
#include "ui/document/document.h"

void UI_RenderHtmlDomBody(uiDocument_t *document, uiHtmlDom_t *dom) {
}

void UI_RenderHtmlDomTitle(uiDocument_t *document, uiHtmlDom_t *dom) {
    C_UtilLogI("%s %s", dom->title, dom->content);
}
