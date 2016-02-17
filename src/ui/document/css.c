#include "ui/ui.h"

#include "core/extern.h"
#include "ui/extern.h"

static dict *uiCssPropertyInfoDict;
static uiCssPropertyInfo_t uiCssPropertyInfoTable[] = {
    {"background-color", UICSS_PROPERTY_TYPE_BACKGROUND_COLOR},
    {"color",            UICSS_PROPERTY_TYPE_COLOR},
    {"padding",          UICSS_PROPERTY_TYPE_PADDING},
    {"margin",           UICSS_PROPERTY_TYPE_MARGIN},
    {"display",          UICSS_PROPERTY_TYPE_DISPLAY},
    {"text-align",       UICSS_PROPERTY_TYPE_TEXT_ALIGN},
    {0},
};

void UI_PrepareCss() {
    uiCssPropertyInfoDict = dictCreate(&stackStringTableDictType, 0);
    for (uiCssPropertyInfo_t *domInfo = &uiCssPropertyInfoTable[0]; 0 != domInfo->name; domInfo++) {
        dictAdd(uiCssPropertyInfoDict, domInfo->name, domInfo);
    }
}

uiCssStyleSheet_t* UI_ParseCssStyleSheet(char *css) {
    return 0;
}
