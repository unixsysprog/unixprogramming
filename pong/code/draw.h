

#define	WALL_VERT_SYM	        '|'
#define WALL_HORIZ_SYM          '-'
#define DFL_BALL_SYM            'O'
#define DFL_PADDLE_SYM          '#'
#define BLANK                   ' '

void curses_init();
void curses_winsz(int* rows, int* cols);
void curses_mvch(int y, int x, char sym);
void curses_refresh();
void curses_print(int y, int x, char* msg);
void curses_release();
