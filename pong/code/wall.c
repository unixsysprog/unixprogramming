#include    <stdio.h>
#include    <unistd.h>
#include    <stdlib.h>
#include        <sys/ioctl.h>
#include        <errno.h>
#include        <string.h>
#include        "draw.h"
#include        "wall.h"

#define MIN_HEIGHT              15
#define MIN_WIDTH               15
#define WALL_LEFT_EDGE           3
#define WALL_TOP_EDGE            3

static void create_wall(int , int);
static void draw_wall();
static void get_ttysize(int* , int* );
static void add_border(int , int, int, int, char);

static struct ppwall wall;

/*wall_init - initialize the wall and draw it */
void wall_init()
{
    int rows, cols;
    curses_winsz(&rows,&cols);     /* uses curses LINES,COLS to get screen size*/

    if (rows == 0 && cols == 0) {
        get_ttysize(&rows,&cols);  /* if above fails then use ioctl()*/
    }
    /* if the screen does not have min hieght and width
    exit the game*/
    if (rows < MIN_HEIGHT || cols < MIN_WIDTH ) {
        fprintf(stderr,"Not enough size to run the game.\n");
        exit(1);
    }
    create_wall(rows, cols);
}


void create_wall(int rows, int cols)
{

    wall.xmin = WALL_LEFT_EDGE;
    wall.ymin = WALL_TOP_EDGE;
    wall.xmax = cols - WALL_LEFT_EDGE;
    wall.ymax = rows - WALL_TOP_EDGE;

    draw_wall();
}


/* draw_wall - draws the border*/
void draw_wall()
{
    int left, right;
    int top,bottom;

    left  = wall.xmin;
    right = wall.xmax;
    top   = wall.ymin;
    bottom = wall.ymax;

                                   //top edge
    add_border(left,right,top,top,WALL_HORIZ_SYM);
                                   //left edge
    add_border(left,left,top,bottom,WALL_VERT_SYM);
                                   //bottom edge
    add_border(left,right,bottom,bottom,WALL_HORIZ_SYM);
}


/* add border - draws lines using curses and symbol passed*/
void add_border(int x1, int x2, int y1, int y2, char symbol)
{
    int i,j;
    for (i = x1; i <= x2 ;i++) {
        for (j = y1; j <= y2; j++) {
            curses_mvch(j,i,symbol);
        }
    }

}


/* get_ttysize - if curses did not return size of screen
                 call this function. This one uses ioctl()
*/
void get_ttysize(int* rows, int* cols)
{

    #ifdef TIOCGWINSZ
    struct winsize winfo;
    printf("getting rows, cols of terminal.\n");
    if (ioctl(0,TIOCGWINSZ,&winfo) == -1) {
        fprintf(stderr,"pong: `%s'", strerror(errno));
        exit(1);
    }
    *rows = winfo.ws_row;
    *cols = winfo.ws_col;
    printf("rows: %u, cols: %u\n",*rows,*cols);
    #endif

}


/* wall_xmin-returns left edge x of wall */
int wall_xmin(){return wall.xmin;}
/* wall_xmax -returns right edge x of wall*/
int wall_xmax(){return wall.xmax;}
/* wall_ymin - returns top edge of wall*/
int wall_ymin(){return wall.ymin;}
/* wall_ymax - returns bottom edge of wall*/
int wall_ymax(){return wall.ymax;}
