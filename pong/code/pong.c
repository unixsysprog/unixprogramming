#include    <stdio.h>
#include    <curses.h>
#include    <signal.h>
#include    <unistd.h>
#include    <stdlib.h>
#include    <string.h>
#include    <errno.h>
#include    <time.h>
#include    <limits.h>
#include    "draw.h"
#include    "wall.h"
#include    "ball.h"
#include    "paddle.h"
#include    "alarmlib.h"

#define  CONV_MIN   60
#define  NUM_BALLS  3

static void set_up();
static void serve();
static void wrap_up();
static void status_msg();
static void iterate();
static void interrupt_handler();
static void update_queue_gametimer( );

static int         balls_left;     /* number of balls allowed in a game */
static int         qcounter = 0;   /* counter to queue the alarm signal*/                   
static long        game_timer = 0; /* timer to show game duration */
//static time_t        start;

int main()
{
    int c;

    set_up();
    serve();

    balls_left  =  NUM_BALLS;      /*set the number of balls allowed in the game*/

    status_msg();                  /*write the initial message for the game at the top*/

    while (balls_left > 0 && (c = getch()) != 'Q') {
        if (c == 'k')              /* if 'k' is pressed move the paddle up*/
            paddle_up();
        else if (c == 'm')         /* if 'm' is presses move the paddle down */
            paddle_down();
                                   /* else just iterate through all alarm signals
                                      queued to move the ball */
        iterate();
    }

    status_msg();                  /* write the final message before ending */
    wrap_up();                     /* clean up*/

    return 0;
}


/*
  set_up - initializes the curses library
           initializes wall, ball, paddle objects
	   calls functions that draws the court,paddle
	   and puts the ball and sets its velocity
	   sets up handler for alaram signal
	   sets the ticker to action.
 */
void set_up()
{
    struct sigaction handler;

    curses_init();

    wall_init();
    paddle_init();
    ball_init();

    curses_refresh();

    memset(&handler,0,sizeof(struct sigaction));
    handler.sa_handler = update_queue_gametimer;
    sigaction(SIGALRM,&handler, NULL);
    signal(SIGINT, interrupt_handler);

    set_ticker(1000/TICKS_PER_SEC);
}


/*
  serve - sets the seed for srand
           and initializes the ball velocity in x and y direction.
*/
void serve()
{
    srand(getpid());
    ball_velocity_init();

}


/*
  releases everything relates to curses
  releases the ticker to no longer send alarms
*/
void wrap_up()
{
    set_ticker(0);
    curses_release();
}


/*
  update_queue_gametimer - callback function when alaram signal occurs
                         - if there are no balls left then do nothing
	                 - else queue it to counter to move the ball later
			 when no paddle movement is under process
			 also increment the number of ticks. When number of ticks
			 are equal to TICKS_PER_SEC increment the game_timer which is in
			 seconds. and update the status msg with the time.
*/
void update_queue_gametimer()
{
    static int nticks;             /* counter for number of ticks*/
    if (balls_left <=0)
        return;

    qcounter++;
    nticks++;
    /* when nticks aggregate to 1 sec ,
    update the game timer and display it and reset the nticks*/
    if (nticks ==  TICKS_PER_SEC) {
        game_timer++;
        status_msg();
        nticks = 0;
    }

}


/*
 status_msg  -display the game status at the top of the court.
              displays number of balls left and game duration in MIN : SEC
 */

void status_msg()
{
    char msg[LINE_MAX] = "";       /*string to hold the game status message*/
    long secs;
    long  mins;

    /*time(&cur_time);
      diff = (long) difftime(cur_time,start); */

    mins = game_timer / CONV_MIN;  /*convert the game_timer to mins*/
    secs = game_timer % CONV_MIN;  /* remainder be the seconds*/

    if (balls_left == 0) {
        sprintf(msg,"GAME OVER.USED ALL THE BALLS.\tTOTAL TIME: %02ld : %02ld ",
            mins, secs);
        curses_print(0,0,msg);
    }
    else {
        sprintf(msg," BALLS LEFT: %d \t TOTAL TIME: %02ld : %02ld",
            balls_left, mins, secs);

        curses_print(0,0,msg);     /*internally calls curses printw*/
    }

    curses_refresh();
}


void iterate()
{
    while (qcounter > 0) {         /* process the queue*/
        ball_move(&balls_left);    /* move the ball and decrement the queue*/
        qcounter--;
    }
}


/*
  interrupt_handler - call back for SIGINT
                       reset the signal handlers and release everything.
*/
void interrupt_handler()
{

    curses_print(0,0,"GAME INTERRUPTED.");
    curses_refresh();
    wrap_up();
    signal(SIGINT, SIG_DFL);
    signal(SIGALRM, SIG_DFL);
    exit(1);
}
