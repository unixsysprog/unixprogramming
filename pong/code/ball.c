#include    <stdio.h>
#include    <unistd.h>
#include    <stdlib.h>
#include        "draw.h"
#include        "ball.h"
#include        "wall.h"
#include        "paddle.h"

#define   MAX_RAND       10        /* maximum random number allowed*/
#define   BALL_BOUNCE    1         /* ball is at the wall or paddle*/
#define   BALL_LOST     -1         /* ball out of court*/
#define   BALL_INCOURT   0         /* ball inside the court*/

static void reinit_ball(int* );
static int bounce_or_lose();
static void handle_paddle_bounce();

static struct ppball ball;

/*
  ball_init -initializes the ball position and
         and puts it in the middle of the court.
*/
void ball_init()
{

    ball.y_pos = (wall_ymax() - wall_ymin())/2;
    ball.x_pos = (wall_xmax() - wall_xmin())/2;

    ball.y_dir = 1  ;
    ball.x_dir = 1  ;
    ball.symbol = DFL_BALL_SYM ;
    ball.prev_symbol = BLANK;

    curses_mvch(ball.y_pos,ball.x_pos,ball.symbol);

}


/*
  ball_velocity_init -
                       sets the xdelay and ydelay randomly
               max random numbers are selected based on
               ratio of width and height of court.
               random number for xdelay can be anything between
               0 and (width)/MAX_RAND
               random number for ydelay can be anything between
               0 and (width+height)/MAX_RAND
 */
void ball_velocity_init()
{
    int width = wall_xmax()-wall_xmin();
    int height = wall_ymax()-wall_ymin();
    int xmax = width/MAX_RAND;
    int ymax = (width + height)/MAX_RAND;

    while(ball.y_delay == 0)
        ball.y_delay = ball.y_count = rand()%ymax;

    while(ball.x_delay == 0)
        ball.x_delay = ball.x_count = rand()%xmax;

}


/*
  ball_move -
            @param nballs - number of remaining balls.
        updates the x and y position of the ball based on the respective
        delays and moves the ball.
        checks if the ball is bouncing of the wall or out of the court
        if the ball goes out of the court, reinitializes another ball.
        bounces only the ball is drawn on the wall or paddle
        and reset back when bounces back.

 */
void ball_move(int* nballs)
{
    int y_cur, x_cur, moved;

    y_cur = ball.y_pos ;           /* old spot*/
    x_cur = ball.x_pos ;
    moved = 0 ;

    /*if (x_cur == wall_xmax() &&
    paddle_contact(y_cur,x_cur) == 1){
      handle_paddle_bounce();
      ball.x_pos += ball.x_dir;
      ball.x_count += ball.x_delay;
      moved = 1;
      }*/

    /* dont move the ball till the count is decremented*/
    if (ball.y_delay > 0 && --ball.y_count == 0 ) {
        ball.y_pos += ball.y_dir ; /* move	*/
                                   /* reset*/
        ball.y_count = ball.y_delay  ;
        moved = 1;
    }

    if (ball.x_delay > 0 && --ball.x_count == 0 ) {
        ball.x_pos += ball.x_dir ; /* move	*/
                                   /* reset*/
        ball.x_count = ball.x_delay  ;
        moved = 1;
    }

    if (moved) {
        /* move the curser to balls old position
        and set the prev_symbol stored in ball*/
        curses_mvch(y_cur, x_cur, ball.prev_symbol);

        /* move the curser to new ball position
        and set the ball symbol */
        curses_mvch(ball.y_pos, ball.x_pos, ball.symbol);

                                   //check for bounce or lose
        if (bounce_or_lose() == -1) {
            reinit_ball(nballs);   //if lost then reinit a new ball
        }

        curses_refresh();
    }

}


/*
  bounce_or_lose
       - returns 1  if the ball is hitting the paddle or wall
       - return -1  if the ball is at the right edge and missing the paddle
       - else returns 0
       - if ball.xpos == paddle.xpos and ball.ypos is in between
             paddle bottom and top bricks then it is a bounce
             if ball.xpos == wall left edge then it is a bounce
             if ball.ypos == wall top edge then it is a bounce
	     if ball.ypos == wall bottom edge then it is a bounce
             if ball.xpos == paddle.xpos and ball.ypos is not in between
	     paddle bottom and top ricks then the ball is lost.
	     when ball bounces changes the corresponding ball direction.
*/
int bounce_or_lose()
{
    int return_val = BALL_INCOURT ;
    int pad_contact = 0;

    ball.prev_symbol = BLANK;
    pad_contact = paddle_contact(ball.y_pos, ball.x_pos);

    if (pad_contact == 1) {        /*ball ypos is in between paddle bricks*/
        handle_paddle_bounce();
        return_val = BALL_BOUNCE;
    }
    else if (pad_contact == -1) {  /* ball ypos is not in between paddle bricks*/
        return_val = BALL_LOST;
    }
    else {
                                   /* top edge*/
        if (ball.y_pos == wall_ymin() ) {
            ball.y_dir = 1 ;
                                   /* restore this symbol in previous loc  */
            ball.prev_symbol = WALL_HORIZ_SYM;
            return_val = BALL_BOUNCE ;
        }
                                   /* bottom edge*/
        else if (ball.y_pos == wall_ymax() ) {
            ball.y_dir = -1 ;
                                   /* restore this symbol in previous loc  */
            ball.prev_symbol = WALL_HORIZ_SYM;
            return_val = BALL_BOUNCE;
        }
                                   /*left edge*/
        if (ball.x_pos == wall_xmin() ) {
            ball.x_dir = 1 ;
                                   /* restore this symbol in previous loc  */
            ball.prev_symbol = WALL_VERT_SYM;
            return_val = BALL_BOUNCE;
        }
    }

    return return_val;
}


/*
  reinit_ball -
          @param nballs- remaining balls
          if there are remaining balls,
          then reinit the ball and randomize the velocity
          i.e. x_delay and y_delay members
 */

void reinit_ball(int* nballs)
{
    curses_mvch(ball.y_pos, ball.x_pos,BLANK);
    *nballs -= 1;

    if (*nballs > 0) {
        ball_init();
        ball_velocity_init();
    }
}


/*
  handle_paddle_bounce - reset the delays and randomize to new
                         delay when ball bounces from paddle.
			 change the direction.
 */

void handle_paddle_bounce()
{
    ball.x_dir = -1;
    ball.x_delay = 0 ;
    ball.y_delay = 0;
                                   /* restore this symbol in previous loc  */
    ball.prev_symbol = DFL_PADDLE_SYM;
    ball_velocity_init();

}
