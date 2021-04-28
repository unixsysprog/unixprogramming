#include	<stdio.h>
#include	<curses.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include        <string.h>
#include        <errno.h>
#include        <aio.h>
#include        "wall.h"
#include        "ball.h"
#include        "paddle.h"
#include        "alarmlib.h"


static void set_up();
static void serve();
static void wrap_up();
static void move_pong(int );
static void move_paddle(int );
static void on_input();
static void setup_aio_buffer();
static void status_msg();

int           *balls_left;    /* number of balls allowed in a game */
int           done = 0;       /* to find if the game is still running*/
int           mvpaddle = 0;   /* to declare paddle is about to move*/
int           mvball = 0;     /* to declare ball is about to move */
int           qcounter = 0;   /* counter to queue the alarm signal*/
struct aiocb  kbcbuf;         /* control buffer for async read*/

int main()
{
    set_up();
    serve();
    balls_left = malloc(sizeof(int));
    *balls_left  =  NUM_BALLS;

    while (!done){
      while(qcounter > 0){
	move_pong(0);
	qcounter--;
      }
    }
    /*while (*balls_left > 0 && (c = getch()) != 'Q'){
      if (c == 'k')
	paddle_up();
      else if (c == 'm')
	paddle_down();
    
	}*/

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
    
    initscr();
    clear();
    noecho();
    cbreak();

    signal(SIGIO, on_input);
    setup_aio_buffer();
    aio_read(&kbcbuf);

    wall_init();
    paddle_init();
    ball_init();
    
    refresh();

    signal(SIGALRM,move_pong);
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
    endwin();
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
   
    while (qcounter > 0){
      move_pong(0);
      qcounter--;
    }
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
	      and reset the callback for alarm signal.
 */
void move_pong(int s)
{
    if (*balls_left <=0)     
      return;    

    if (mvpaddle == 1){
      qcounter++;
      return;
    }
    if (mvball == 1){
      qcounter++;
      return;
    }
    mvball = 1;
    ball_move(balls_left);
    status_msg();
    mvball = 0;
    signal(SIGALRM, move_pong);		/* re-enable handler	*/
}


void status_msg()
{
    if (*balls_left == 0){
      mvprintw(0,0," Game over. Used up all balls. \n");
    }
    else{
      mvprintw(0,0," BALLS LEFT: %d \t TOTAL TIME: ",
	       *balls_left);
    }
    clrtoeol();
    refresh();

}

