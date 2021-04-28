#include	<stdio.h>
#include	<curses.h>
#include	<fcntl.h>
#include	<signal.h>

/* #define	POLLED		/* for steady motion and polled keyboard */
#define	INTERRUPT	/* blocked reads and alarm-driven action */

/*
 *	bounce 1.0	polled or interrupt version
 *
 *	bounce a character (default is *) around the screen
 *	defined by some parameters
 *
 *	user input:
 *		 	s slow down x component, S: slow y component
 *		 	f speed up x component,  F: speed y component
 *			Q quit
 *
 *	polled:
 *	runs a main loop to drive ball and check keyboard for input
 *	after each tick.  Has some serious problems.
 *
 *	interrupt:
 *	blocks on read, but timer tick sets SIGALRM which are caught
 *	by ball_move
 */

#include	"bounce.h"

struct ppball the_ball ;

void	ball_move(int);

/**
 **	the main loop
 **/

int
main()
{
	int	c;

	set_up();

	while ( ( c = gitachar()) != 'Q' ){
		if ( c == 'f' )	     the_ball.x_delay--;
		else if ( c == 's' ) the_ball.x_delay++;
		else if ( c == 'F' ) the_ball.y_delay--;
		else if ( c == 'S' ) the_ball.y_delay++;
#ifdef	POLLED
		ball_move(0);		/* if not polled, SIGALRM calls this */
#endif
	}

	wrap_up();
	return 0;
}

gitachar()
/*
 *	in the POLLED version, put the tty into nodelay mode
 *	and call getchar(), then reset it so that the output
 *	function doesn't get bytes all over its face
 *	in the INTERRUPT version, block for input since the
 *	timer tick drives the ball, not the loop
 */
{
	int	c;

#ifdef	POLLED
	blocking( 1 );
#endif
	c = getchar();
#ifdef	POLLED
	blocking( 0 );
#endif
	return c;
}

set_up()
/*
 *	init structure and other stuff
 */
{
	the_ball.y_pos = Y_INIT;
	the_ball.x_pos = X_INIT;
	the_ball.y_count = the_ball.y_delay = Y_TTM ;
	the_ball.x_count = the_ball.x_delay = X_TTM ;
	the_ball.y_dir = 1  ;
	the_ball.x_dir = 1  ;
	the_ball.symbol = DFL_SYMBOL ;

	initscr();		/* will exit if any problems	*/
	noecho();		/* set tty mode			*/
	crmode();		/* -icanon			*/

	signal( SIGINT , SIG_IGN );
	mvaddch( the_ball.y_pos, the_ball.x_pos, the_ball.symbol  );
	refresh();

#ifdef	INTERRUPT
	signal( SIGALRM, ball_move );
	set_ticker( 1000 / TICKS_PER_SEC );	/* send millisecs per tick */
#endif
}

/*
 * function: blocking(how)
 *  purpose: put tty in no-delay mode.  This is one way of
 *           doing two things at once.  If no char has been typed,
 *	     getchar() returns at once.
 *     args: how - if 1 =>: set no delay, if 0 => set regular mode
 */
blocking(int what_to_do)
{
	static	int	flags, oldflags;

	if ( what_to_do == 1 )
	{
		oldflags = flags = fcntl( 0, F_GETFL , 0 );	
		flags |= O_NDELAY ;
		fcntl( 0 , F_SETFL , flags );
	}
	else
		fcntl( 0, F_SETFL, oldflags );
}

/*
 * restore the tty driver 
 */
wrap_up()
{
#ifdef	INTERRUPT
	set_ticker( 0 );
#endif
	nocrmode();		/* reset canonical mode	*/
	echo();			/* yes echo		*/
	endwin();		/* put back to normal	*/
}


/*
 * function: ball_move
 *  purpose: move the ball one timer tick worth of distance
 *   action: actually it decrements each of the two timers
 *           in the ball.  If either one hits zero, then
 *  	     the ball moves its x or y position by one
 *	     unit and then passes the ball to the bounce
 *	     function
 *     args: s --  ignored, but signal handlers are passed signum
 */

void
ball_move(int s)
{
	int	y_cur, x_cur, moved;

#ifdef	INTERRUPT
	signal( SIGALRM , SIG_IGN );		/* dont get caught now 	*/
#endif
	y_cur = the_ball.y_pos ;		/* old spot		*/
	x_cur = the_ball.x_pos ;
	moved = 0 ;

	if ( the_ball.y_delay > 0 && the_ball.y_count-- == 1 ){
		the_ball.y_pos  += the_ball.y_dir ;	/* move	*/
		the_ball.y_count = the_ball.y_delay  ;	/* reset*/
		moved = 1;
	}

	if ( the_ball.x_delay > 0 && the_ball.x_count-- == 1 ){
		the_ball.x_pos  += the_ball.x_dir ;	/* move	*/
		the_ball.x_count = the_ball.x_delay  ;	/* reset*/
		moved = 1;
	}

	if ( moved ){
		mvaddch( y_cur, x_cur, BLANK );
		mvaddch( the_ball.y_pos, the_ball.x_pos, 
				the_ball.symbol  );
		bounce_or_lose( &the_ball );
		move(LINES-1,COLS-2);
		refresh();
	}
#ifdef 	INTERRUPT
	signal( SIGALRM, ball_move);
#endif

}

/*
 * call this function to see if the ball at *bp
 * is at a bounce surface .  Right not it returns
 * 1 if the ball has bounced.
 */

bounce_or_lose(struct ppball *bp)
{
	int	return_val = 0 ;

	if ( bp->y_pos == TOP_ROW )
		bp->y_dir = 1 , return_val = 1 ;
	else if ( bp->y_pos == BOT_ROW )
		bp->y_dir = -1 , return_val = 1;

	if ( bp->x_pos == LEFT_EDGE )
		bp->x_dir = 1 , return_val = 1 ;
	else if ( bp->x_pos == RIGHT_EDGE )
		bp->x_dir = -1 , return_val = 1;

	return return_val;
}

