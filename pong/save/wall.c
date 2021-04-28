#include	<stdio.h>
#include	<curses.h>
#include	<unistd.h>
#include	<stdlib.h>
#include        <sys/ioctl.h>
#include        <errno.h>
#include        <string.h>
#include        "wall.h"



#define	WALL_LEFT_EDGE	         3
#define	WALL_TOP_EDGE	         3

static void create_wall(int , int);
static void draw_wall();
static void get_ttysize(int* , int* );
static void add_border(int , int, int, int, char);

struct ppwall wall;

void wall_init()
{
   int rows, cols;
   get_ttysize(&rows,&cols);
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

void draw_wall()
{
    int left, right;
    int top,bottom;

    left  = wall.xmin; 
    right = wall.xmax;
    top   = wall.ymin; 
    bottom = wall.ymax;

    //printf("WALL BOUNDARY:left: %d ; right: %d ; top: %d; bottom: %d; \n",
    //	   left,right,top, bottom);

    add_border(left,right,top,top,WALL_HORIZ_SYM);//top edge
    add_border(left,left,top,bottom,WALL_VERT_SYM);//left edge
    add_border(left,right,bottom,bottom,WALL_HORIZ_SYM);//bottom edge
}

void add_border(int x1, int x2, int y1, int y2, char symbol)
{
  int i,j;
  //printf("ADD BORDER:x1: %d ; x2: %d ; y1: %d; y2: %d; c: %c\n",
  //	 x1,x2,y1,y2,symbol);

  for (i = x1; i <= x2 ;i++){
    for (j = y1; j <= y2; j++){
      move(j,i);
      addch(symbol);
    }
  }

  //refresh();

}

void get_ttysize(int* rows, int* cols)
{
    struct winsize winfo;
    printf("getting rows, cols of terminal.\n");
    if (ioctl(0,TIOCGWINSZ,&winfo) == -1){
       fprintf(stderr,"pong: `%s'", strerror(errno));
       exit(1);
    }
    *rows = winfo.ws_row;
    *cols = winfo.ws_col;
    printf("rows: %u, cols: %u\n",*rows,*cols);
}


int wall_xmin(){return wall.xmin;}
int wall_xmax(){return wall.xmax;}
int wall_ymin(){return wall.ymin;}
int wall_ymax(){return wall.ymax;}
