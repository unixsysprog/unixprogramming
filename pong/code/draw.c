#include    <stdio.h>
#include    <curses.h>
#include    <unistd.h>
#include    <stdlib.h>
#include        "draw.h"

/* This source code is just a wrapper for curses functionality
    using draw.h in ball, paddle, wall source files
    instead of refering to curses*/

void curses_init()
{
    initscr();
    clear();
    noecho();
    cbreak();

}


/* curses_winsz - gets screen size*/
void curses_winsz(int* rows, int* cols)
{
    *rows = LINES;
    *cols = COLS;

}


/*curses_mvch - saves the previous curser location
                calls move and addch of curses on new passed y,x
		restores the curser position
*/
void curses_mvch(int y, int x, char symbol)
{
    int x_actual, y_actual;

    getyx(stdscr, y_actual,x_actual);
    move(y,x);
    addch(symbol);

    move(y_actual,x_actual);
}


/*curses_refresh - calls curser refresh*/
void curses_refresh()
{
    refresh();
}


/* curses_print - calls move and printw of curses.
                  but before that it saves the curser location
		  and restores it at the end
*/
void curses_print(int y, int x, char* msg)
{
    int x_actual, y_actual;
    getyx(stdscr,y_actual,x_actual);
    move(y,x);
    printw(msg);
    clrtoeol();
    move(y_actual,x_actual);
}


void curses_release()
{
    endwin();

}
