#include "ui/ui.h"

#include "core/extern.h"
#include "ui/extern.h"

static dict *UICssPropertyInfoDict;
static UICssPropertyInfo UICssPropertyInfoTable[] = {
    {"background-color", UICSS_PROPERTY_TYPE_BACKGROUND_COLOR},
    {"color",            UICSS_PROPERTY_TYPE_COLOR},
    {"padding",          UICSS_PROPERTY_TYPE_PADDING},
    {"margin",           UICSS_PROPERTY_TYPE_MARGIN},
    {"display",          UICSS_PROPERTY_TYPE_DISPLAY},
    {"text-align",       UICSS_PROPERTY_TYPE_TEXT_ALIGN},
    {0},
};

void UIPrepareCss() {
    UICssPropertyInfoDict = dictCreate(&stackStringTableDictType, 0);
    for (UICssPropertyInfo *domInfo = &UICssPropertyInfoTable[0]; 0 != domInfo->name; domInfo++) {
        dictAdd(UICssPropertyInfoDict, domInfo->name, domInfo);
    }
}
