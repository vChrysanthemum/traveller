#include "core/sds.h"
#include "core/adlist.h"
#include "core/dict.h"
#include "core/util.h"
#include "core/extern.h"

#include "event/event.h"
#include "ui/ui.h"

#include "ui/extern.h"

int UI_PrepareColor() {
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
            ui_ColorPair[types[i]][types[j]] = (types[i]+1)*100 + (types[j]+1);
            init_pair(ui_ColorPair[types[i]][types[j]], types[i], types[j]);
        }
    }

    return 0;
}
