	
/**
 **	struct for ball
 **/

struct ppball {
  int	x_pos, x_dir,    
	y_pos, y_dir,
	y_delay, y_count,
	x_delay, x_count;
  char	symbol ;
  char  prev_symbol;  /*stores the prev symbol at the
			previous ball location to
                        restore it back*/

} ;


void ball_init();
void ball_velocity_init();
void ball_move(int* );

