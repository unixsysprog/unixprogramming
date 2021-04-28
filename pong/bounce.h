
/**
 **	some parameters
 **/

#define	BLANK		' '
#define	DFL_SYMBOL	'O'
#define	TOP_ROW		5
#define	BOT_ROW 	20
#define	LEFT_EDGE	10
#define	RIGHT_EDGE	70
#define	X_INIT		10		/* starting col		*/
#define	Y_INIT		10		/* starting row		*/
#define	TICKS_PER_SEC	50		/* affects speed	*/

#ifdef POLLED
#define	X_DELAY		20		/* ticks to move for X	*/
#define	Y_DELAY		32		/* ticks to move for Y	*/
#endif

#ifdef	INTERRUPT
#define	X_DELAY		5
#define	Y_DELAY		8
#endif

/**
 **	the only object in town
 **/

struct ppball {
		int	y_pos, x_pos,
			y_delay, x_delay,
			y_count, x_count,
			y_dir, x_dir;
		char	symbol ;

	} ;
