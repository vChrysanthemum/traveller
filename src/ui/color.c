#include "ui/ui.h"

#include "core/extern.h"
#include "ui/extern.h"

void UIPrepareColor() {
    start_color();
    init_pair(CP_CONSOLE_TAB_ACTIVE, COLOR_GREEN, COLOR_BLUE);
    init_pair(CP_CONSOLE_TAB, COLOR_BLACK, COLOR_WHITE);
    init_pair(CP_CONSOLE_TAB_BG, COLOR_BLACK, COLOR_WHITE);

    int types[8] = {
        COLOR_BLACK, 
        COLOR_RED,
        COLOR_GREEN,
        COLOR_YELLOW,
        COLOR_BLUE,
        COLOR_MAGENTA,
        COLOR_CYAN,
        COLOR_WHITE
    };
    
    int i, j;
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            UIColorPair[types[i]][types[j]] = (types[i]+1)*100 + (types[j]+1)*10;
            init_pair(UIColorPair[types[i]][types[j]], types[i], types[j]);
        }
    }

    UIColorPairTable = dictCreate(&stackStringTableDictType, 0);
    dictAdd(UIColorPairTable, "black",   (void*)&UICOLOR_BLACK);
    dictAdd(UIColorPairTable, "red",     (void*)&UICOLOR_RED);
    dictAdd(UIColorPairTable, "green",   (void*)&UICOLOR_GREEN);
    dictAdd(UIColorPairTable, "yellow",  (void*)&UICOLOR_YELLOW);
    dictAdd(UIColorPairTable, "magenta", (void*)&UICOLOR_MAGENTA);
    dictAdd(UIColorPairTable, "cyan",    (void*)&UICOLOR_CYAN);
    dictAdd(UIColorPairTable, "white",   (void*)&UICOLOR_WHITE);
}
