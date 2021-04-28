/** Modified from pages 470-472 _Intro_to_the_Window_X_System_ **/
/** by Oliver Jones                                            **/

/*
 * compile this program as:
 *
 *   cc hello.world.c -lX11 -o hello.world
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>

char hello[] = "Hello World!";
char hi[] = "hi";
char instructions[] = "  Press q to quit.  Click to add messages";

main( int argc, char **argv )
{
	Display      *mydisplay;
	Window        mywindow;
	GC            mygc;
	XEvent        myevent;
	KeySym        mykey;
	XSizeHints    myhint;
	int           myscreen;
	unsigned long myforeground,
	mybackground;
	int           i, done;
	char          text[10];

	/**********SETUP X ENVIRONMENT*************/

	mydisplay = XOpenDisplay( "" );            /* Open TCP/IP connection */
	myscreen  = DefaultScreen( mydisplay );

	mybackground = WhitePixel( mydisplay, myscreen );/* get color values */
	myforeground = BlackPixel( mydisplay, myscreen );

	myhint.x = 200;     
	myhint.y = 300;              /* fill in hints for      */
	myhint.width = 350; 
	myhint.height = 250;         /* window manager         */
	myhint.flags = PPosition | PSize;

	mywindow = XCreateSimpleWindow( mydisplay, 
					RootWindow( mydisplay, myscreen ),
					myhint.x, myhint.y,
				        myhint.width, myhint.height,
	    				5, myforeground, mybackground );

	/* Tell window manager    */

	XSetStandardProperties( 	mydisplay, 
					mywindow, 
					hello, hello, None,
				argv, argc, &myhint );   

	mygc = XCreateGC(mydisplay, mywindow, 0, 0);   /* default from parent */
	XSetBackground(mydisplay, mygc, mybackground); /* values we use when we  */
	XSetForeground(mydisplay, mygc, myforeground); /* try to do something */

	XSelectInput( mydisplay, mywindow,           /* Events that we want  */
			ButtonPressMask|KeyPressMask|ExposureMask );

	/**************END OF SETUP******************/

	XMapRaised( mydisplay, mywindow );             /* Make window visible */

	done=0;
	do {
		XNextEvent( mydisplay, &myevent );     /* wait for event      */
		switch( myevent.type ) 
		{

		case Expose:
			if( myevent.xexpose.count==0 )
				XDrawImageString( myevent.xexpose.display, 
						  myevent.xexpose.window,
					          mygc, 50, 50, 
						  hello, strlen( hello ));
			break;

		case MappingNotify:
			XRefreshKeyboardMapping( &myevent );
			break;

		case ButtonPress:
			XDrawImageString( myevent.xexpose.display, 
					  myevent.xexpose.window, mygc,
					  myevent.xbutton.x, myevent.xbutton.y,
			    		  hi, strlen(hi));
			break;

		case KeyPress:
			i = XLookupString( &myevent, text, 10, &mykey, 0 );
			if( i==1 && text[0]=='q' ) done = 1;
			break;
		}
	} 
	while( done==0 );

	/*****************CLEAN UP before leaving*************/

	XFreeGC( mydisplay, mygc );
	XDestroyWindow( mydisplay, mywindow );
	XCloseDisplay( mydisplay );
	exit( 0 );
}
