#include	<stdio.h>
#include	<curses.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include        "paddle.h"
#include        "wall.h"


struct pppaddle  paddle;
static void draw_paddle();

/*
  paddle_init - initializes the struct pppaddle
                with wall right col and height equal to 1/3 the 
		height of the court.
		draws the paddle on the screen.
*/
void paddle_init()
{
    
    paddle.pad_top = wall_ymin();
    paddle.pad_bot = wall_ymin() + (wall_ymax() - wall_ymin())/3;
    paddle.pad_col = wall_xmax();
    paddle.pad_char = DFL_PADDLE_SYM;

    draw_paddle(paddle.pad_top,paddle.pad_bot);
    // printf("DRAW PADDLE: paddle.pad_top= %d; paddle.pad_bot= %d\n",
    //	    paddle.pad_top, paddle.pad_bot);
}


/*
  paddle_up - this is called when user presses 'k' on the keyboard
              replaces the bottom most character with blank char
	      put a # at the top
	      if the paddle is at the topmost edge of the wall
	      the method will just return.
 */
void paddle_up()
{
     if (paddle.pad_top == wall_ymin())
       return;

     move(paddle.pad_bot,paddle.pad_col);
     addch(BLANK);

     paddle.pad_top -= 1;
     paddle.pad_bot -= 1;

     move(paddle.pad_top,paddle.pad_col);
     addch(DFL_PADDLE_SYM);
     refresh();
}

/*
  paddle_down - this method is called when user presses 'm' on the keyboard
                replaces the topmost character with blank char
		and puts # at the bottom
		if the paddle is already at the bottom edge then it will
		just return.
 */
void paddle_down()
{
     if (paddle.pad_bot == wall_ymax())
       return;

     move(paddle.pad_top,paddle.pad_col);
     addch(BLANK);

     paddle.pad_top += 1;
     paddle.pad_bot += 1;
     
     
     move(paddle.pad_bot,paddle.pad_col);
     addch(DFL_PADDLE_SYM);
     refresh();
}

/*
  paddle_contact -
                @param y - yposition of the ball
		@param x - x position of the ball
		returns 1 if the ball is touching the paddle
		returns -1 if the ball is at the right most edge
		           and missing the paddle
		returns 0 if the ball is not near paddle
 */
int paddle_contact(int y, int x)
{
     int incontact = 0;

     if (x == paddle.pad_col){
       if (y >= paddle.pad_top && y <= paddle.pad_bot){
	 incontact = 1;
       }
       if (y < paddle.pad_top || y > paddle.pad_bot){
	 incontact = -1;
       }
     }

     return incontact;
    
}


/*
  draw_paddle -
             @param y1 - topmost point of the paddle
	     @param y2 - bottom point of the paddle
	     loops from y1 to y2 and draws # on screen 
	     to indicate it is a paddle
	     x position is the right most column of the court.
 */
void draw_paddle(int y1, int y2)
{
    int i;
    for (i = y1 ; i <= y2; i++){
      move(i, paddle.pad_col);
      addch(paddle.pad_char);
    }
    
}
