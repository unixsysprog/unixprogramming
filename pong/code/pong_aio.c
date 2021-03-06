#include	<stdio.h>
#include	<curses.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include        <string.h>
#include        <errno.h>
#include        <aio.h>
#include        "limits.h"
#include        "draw.h"
#include        "wall.h"
#include        "ball.h"
#include        "paddle.h"
#include        "alarmlib.h"

#define NUM_BALLS  3
#define CONV_MIN   60

static void set_up();
static void serve();
static void wrap_up();
static void move_pong();
static void move_paddle(int );
static void on_input();
static void setup_aio_buffer();
static void status_msg();

static int    balls_left;    /* number of balls allowed in a game */
static int    done = 0;       /* to find if the game is still running*/
static int    mvpaddle = 0;   /* to declare paddle is about to move*/
static int    mvball = 0;     /* to declare ball is about to move */
static int    qcounter = 0;   /* counter to queue the alarm signal*/
struct aiocb  kbcbuf;         /* control buffer for async read*/
static long   game_timer = 0; /* counter for game duration in seconds*/

int main()
{
   
    set_up();
    serve();
    
    balls_left  =  NUM_BALLS;
  
    status_msg();

    while (!done){
      while(qcounter > 0){
	move_pong(0);
	qcounter--;
      }
    }
    status_msg();
    wrap_up();

    return 0;
}



/*
  set_up - initializes the curses library
           sets up the callback function for io signal and alarm signal
	   calls init methods of wall, ball,paddle
	   calls initializing on aiocb
	   sets the ticker
 */
void set_up()
{
    struct sigaction handler;

    curses_init();

    setup_aio_buffer();
    aio_read(&kbcbuf);

    wall_init();
    paddle_init();
    ball_init();
    
    curses_refresh();
    memset(&handler,0,sizeof(struct sigaction));
    handler.sa_handler = move_pong;
    sigaction(SIGALRM,&handler, NULL);
    signal(SIGIO, on_input);

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
  on_input - callback for asynchronous read
             reads the buffer and moves the paddle based on the input
 */
void on_input()
{
    int c;
    char *cp = (char*)kbcbuf.aio_buf;  /* cast to char */

    if (aio_error(&kbcbuf) != 0)
      fprintf(stderr,"pong: `%s'\n",strerror(errno));
    else{
      if (aio_return(&kbcbuf) == 1){
	c = *cp;
	if (c == 'Q')
	  done = 1;
	else if (c == 'k'){
	  move_paddle(c);	  
	}
	else if (c == 'm'){
	  move_paddle(c);
	}
      }
    }
    aio_read(&kbcbuf);            /*place new request */
}


/*
  setup_aio_buffer - initializes the control buffer aiocb
                     sets the file descriptor to stdin and
		     buffer size to 1 and type of signal
 */
void setup_aio_buffer()
{
    static char buf[1];
    kbcbuf.aio_fildes = 0;            /* standard input */
    kbcbuf.aio_buf = buf;              /* buffer */
    kbcbuf.aio_nbytes = 1;            /* number to read */     
    kbcbuf.aio_offset = 0;            /* offset in line */
                                      /*signal when read is ready*/
    kbcbuf.aio_sigevent.sigev_notify = SIGEV_SIGNAL; 
    kbcbuf.aio_sigevent.sigev_signo = SIGIO;
}

/*
  move_paddle - 
             @param c - character passed by user on tty
	     tell everyone that paddle is about to move i.e by setting
	     mvpaddle to 1
	     if alarm signal is raised during paddle movement, it will be queued
	     once the paddle movement is done,tell everyone that paddle is moved
	     by setting mvpaddle =0
	     then look if alaram signal are queued. If there are 
	     call move_ball to update the ball position. process all the 
	     signals that are caught/queued.
 */
void move_paddle(int c)
{
    if (done)
      return;

    mvpaddle = 1;

    if (c == 'k')
      paddle_up();
    else if (c == 'm')
      paddle_down();
     
    mvpaddle = 0;
   
}


/*
  move_pong - callback function when alaram signal occurs
            - if there are no balls left then do nothing
	    - if paddle is moving just queue the signal and return
	    - if already an alarm signal is being processed and we are
	      in middle of moving the ball, then queue the signal and return
	    - if nothing above is happening, declare that we are about to
	      move the ball by setting mvball =1
	      then move the ball calling move_ball
	      after the ball is moved, tell we are done moving the ball 
	      by setting mvball = 0
 */
void move_pong()
{
    static int nticks;
    
    nticks++;
    if (nticks ==  TICKS_PER_SEC){
      game_timer++;
      status_msg();
      nticks = 0;
    }

    if (balls_left <=0) { 
      done = 1;
      return;       
    }

    if (mvpaddle == 1){
      qcounter++;
      return;
    }
    if (mvball == 1){
      qcounter++;
      return;
    }
    mvball = 1;
    ball_move(&balls_left);
    mvball = 0;

}

/*
 status_msg  -display the game status at the top of the court.
              displays number of balls left and game duration in MIN : SEC
 */

void status_msg()
{
    char msg[LINE_MAX] = "";
    long secs;
    long  mins;

    mins = game_timer / CONV_MIN;
    secs = game_timer % CONV_MIN;    

    if (balls_left == 0){
      sprintf(msg,"GAME OVER.USED ALL THE BALLS.\tTOTAL TIME: %02ld : %02ld ",
	      mins, secs);
      curses_print(0,0,msg);
    }
    else{
       sprintf(msg," BALLS LEFT: %d \t TOTAL TIME: %02ld : %02ld",
      	      balls_left, mins, secs);
     
      curses_print(0,0,msg);
    }
   
    curses_refresh();
}
