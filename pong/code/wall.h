

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

/* utility functions*/
int wall_xmin();
int wall_xmax();
int wall_ymin();
int wall_ymax();
