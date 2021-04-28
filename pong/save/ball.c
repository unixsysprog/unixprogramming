#include	<stdio.h>
#include	<curses.h>
#include	<unistd.h>
#include	<stdlib.h>
#include        "ball.h"
#include        "wall.h"
#include        "paddle.h"
#include	<sys/time.h>

#define  MAXXNUM      5
#define  MAXYNUM      8

struct ppball* ball = NULL;

void reinit_ball(int* );
int bounce_or_lose();


/*
  ball_init - 
             creates a new ball (malloc struct ppball)
	     and puts it in the middle of the court.
*/
void ball_init()
{
    ball = malloc(sizeof(struct ppball));
    if (ball == NULL){
      fprintf(stderr,"not enough memory\n");
      exit(1);
    }
    ball->y_pos = (wall_ymax() - wall_ymin())/2;
    ball->x_pos = (wall_xmax() - wall_xmin())/2;

    ball->y_dir = 1  ;
    ball->x_dir = 1  ;
    ball->symbol = DFL_BALL_SYM ;
    ball->prev_symbol = BLANK;
    //printf("INIT BALL:ball.y_pos= %d; ball.x_pos= %d\n",
    //ball->y_pos,ball->x_pos);

    move(ball->y_pos, ball->x_pos);
    addch(ball->symbol);
}

void ball_velocity_init()
{
    int width = wall_xmax()-wall_xmin();
    int height = wall_ymax()-wall_ymin();
    int xmax = width/10;
    int ymax = (width + height)/10;
    
    if (ball == NULL)
      return;

    while(ball->y_delay == 0)
    ball->y_delay = ball->y_count = rand()%ymax;

    while(ball->x_delay == 0)
    ball->x_delay = ball->x_count = rand()%xmax;

}


/*
  ball_move - 
            @param nballs - number of remaining balls.
	    updates the x and y position of the ball based on the respective
	    delays and moves the ball.
	    checks if the ball is bouncing of the wall or out of the court
	    if the ball goes out of the court, the number of remaining 
	    balls are decremented by one. if the remaining balls > 0
	    then a new ball is instantiated and put in the middle of the court.
	    
 */
void ball_move(int* nballs)
{
    int	y_cur, x_cur, moved;

    if (ball == NULL)
      return;
 
    y_cur = ball->y_pos ;		/* old spot		*/
    x_cur = ball->x_pos ;
    moved = 0 ;

    if (ball->y_delay > 0 && --ball->y_count == 0 ){
      ball->y_pos += ball->y_dir ;	/* move	*/
      ball->y_count = ball->y_delay  ;	/* reset*/
      moved = 1;
    }

    if (ball->x_delay > 0 && --ball->x_count == 0 ){
      ball->x_pos += ball->x_dir ;	/* move	*/
      ball->x_count = ball->x_delay  ;	/* reset*/
      moved = 1;
    }

    if (moved){
      mvaddch(y_cur, x_cur, ball->prev_symbol);
      mvaddch(ball->y_pos, ball->x_pos, ball->symbol);       

      if (bounce_or_lose() == -1){	  
	reinit_ball(nballs);	 
      }

      //printf("MOVING BALL: ball.y_pos= %d; ball.x_pos= %d; nballs= %d\n", 
      //         ball->y_pos,ball->x_pos, *nballs);  
      refresh();
    }    

}

/*
  bounce_or_lose
           - returns 1  if the ball is hitting the paddle or wall
	   - return -1  if the ball is at the right edge and missing the paddle
	   - else returns 0 
	   - checks the ball positions with wall boundaries 
	     and paddle boundaries. if it matches then changes the 
	     balls direction accordingly.
 */
int bounce_or_lose()
{
    int	return_val = 0 ;
    int pad_contact = 0;
    
    if (ball == NULL)
      return 0;

    ball->prev_symbol = BLANK;
    pad_contact = paddle_contact(ball->y_pos, ball->x_pos);

    if (pad_contact == 1){
      ball->x_dir = -1;     
      ball->x_delay = 0 ;
      ball->y_delay = 0;
      ball->prev_symbol = DFL_PADDLE_SYM;
      ball_velocity_init();
      return_val = 1;
    }
    else if (pad_contact == -1){
      return_val = -1;
    }
    else{
      if (ball->y_pos == wall_ymin() ){
	ball->y_dir = 1 ;
        ball->prev_symbol = WALL_HORIZ_SYM;
	return_val = 1 ;
      }
      else if (ball->y_pos == wall_ymax() ){
	ball->y_dir = -1 ;
	ball->prev_symbol = WALL_HORIZ_SYM;
	return_val = 1;
      }
      if (ball->x_pos == wall_xmin() ){
	ball->x_dir = 1 ;
	ball->prev_symbol = WALL_VERT_SYM;
	return_val = 1 ;
      }
    }

    return return_val;
}


/*
  reinit_ball -
              @param nballs- remianing balls
	      if there are remaining balls, instantiate
	      another ball and its velocity.
 */

void reinit_ball(int* nballs)
{
    if(ball == NULL)
      return;

    move(ball->y_pos, ball->x_pos);
    addch(BLANK);
    //printf("RELEASE BALL: ball.y_pos= %d; ball.x_pos= %d\n",
    //ball->y_pos,ball->x_pos);
    free(ball);
    ball = NULL;
    *nballs -= 1;

    if (*nballs > 0){
      ball_init();
      ball_velocity_init();
    }
    
}
