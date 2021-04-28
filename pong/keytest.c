#include	<curses.h>

main()
{
	int	c = 0;
	int	row = 10, col = 10;

	initscr();
	clear();
	keypad( stdscr, TRUE );
	while(  c != 'Q' )
	{
		c = getch();
		if ( c == KEY_UP )
			row--;
		else if ( c == KEY_DOWN )
			row++;
		else if ( c == KEY_LEFT )
			col--;
		else if ( c == KEY_RIGHT )
			col++;
		else
			mvprintw(row,col,"Char was %3d", c);
		move(row, col);
		refresh();
	}
	endwin();
}
