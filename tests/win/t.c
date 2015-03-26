#include <ncurses.h>

WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);

int main(int argc, char *argv[])
{  
    WINDOW *my_win;
    int startx, starty, width, height;
    int ch;

    initscr();
    cbreak();
    keypad(stdscr, TRUE);

    /* Start curses mode            */
    /* Line buffering disabled, Pass on
     *  * everty thing to me           */
    /* I need that nifty F1         */
    WINDOW *local_win;
    local_win = newwin(20, 20, 1, 1);
    box(local_win, 0 , 0);
    wrefresh(local_win);
    refresh();
    ch = getch();

    delwin(local_win);
    endwin();
}
