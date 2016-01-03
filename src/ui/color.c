#include "ui/ui.h"

void UIinitColor() {
    start_color();
    init_pair(CP_CONSOLE_TAB_ACTIVE, COLOR_GREEN, COLOR_BLUE);
    init_pair(CP_CONSOLE_TAB, COLOR_BLACK, COLOR_GREEN);
    init_pair(CP_CONSOLE_TAB_BG, COLOR_GREEN, COLOR_WHITE);
}
