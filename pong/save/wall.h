

#define	WALL_VERT_SYM	        '|'
#define WALL_HORIZ_SYM          '-'
#define DFL_BALL_SYM            'O'
#define DFL_PADDLE_SYM          '#'
#define BLANK                   ' '

/**
 **	the wall
 **
 */
struct ppwall {
  int   xmax;
  int   ymax;
  int   xmin;
  int   ymin;

} ;


void wall_init();
int wall_xmin();
int wall_xmax();
int wall_ymin();
int wall_ymax();
