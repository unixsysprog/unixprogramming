#include	<stdio.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include        <string.h>
#include	"wtmplib.h"	/* include interface def for wtmplib.c	*/

#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/

#define MAX_NAME        256



int main(int ac, char **av)
{
        char           fname[MAX_NAME];
        char           uname[MAX_NAME];
        void 	       read_wtmp(char* , char* );

	 if ( ac < 2 || ac > 4 ){

	   fprintf(stderr,"incorrect arguments specified.");
	   exit(1);
        }
       
	 //parse command arguments
	if (strcmp(av[1] , "-f") == 0){

	  sprintf ( fname,"%s",av[2]);
	  sprintf ( uname, "%s" , av[3]);
        }
        else{

	  sprintf ( fname , "%s",  WTMP_FILE );
	  sprintf ( uname , "%s", av[1] );
        }

        read_wtmp( fname, uname);
       
	return 0;
}

/*
 read_wtmp - workhorse to start reading the file .
             opens, reads the file backward sequentially and closes
*/
void read_wtmp(char* file, char* user)
{
        struct utmp	*wtbufp;	/* holds pointer to next rec	*/
	//	*wtmp_next();	/* returns pointer to next	*/
	void		show_info( struct utmp * );

	if ( wtmp_open( file ) == -1 ){
		fprintf(stderr,"cannot open %s\n", file);
		exit(1);
	}

	while ( ( wtbufp = wtmp_readback() ) != ((struct utmp *) NULL) ) {
	  
        if ( wtbufp->ut_type == USER_PROCESS &&
             strcmp( wtbufp->ut_user,user)==0) { 
                                     
	        show_info(wtbufp);   
                break;                      /* got the lastlogin record */  
                     
	   }

	}

	wtmp_close( );
       
}


/*
 *	show info()
 *			displays the contents of the utmp struct
 *			in human readable form
 *			* displays nothing if record has no user name
 */
void
show_info( struct utmp *utbufp )
{
	void	showtime( time_t , char *);

	if ( utbufp->ut_type != USER_PROCESS )
		return;

	printf("%-8s", utbufp->ut_name);		/* the logname	*/
	printf(" ");					/* a space	*/
	printf("%-12.12s", utbufp->ut_line);		/* the tty	*/
        printf("%6d ", utbufp->ut_pid );
	printf(" ");					/* a space	*/

	showtime( utbufp->ut_time, DATE_FMT );		/* display time	*/
	if ( utbufp->ut_host[0] != '\0' )
		printf(" (%s)", utbufp->ut_host);	/* the host	*/
	printf("\n");					/* newline	*/
}

#define	MAXDATELEN	100

void
showtime( time_t timeval , char *fmt )
/*
 * displays time in a format fit for human consumption.
 * Uses localtime to convert the timeval into a struct of elements
 * (see localtime(3)) and uses strftime to format the data
 */
{
	char	result[MAXDATELEN];

	struct tm *tp = localtime(&timeval);		/* convert time	*/
	strftime(result, MAXDATELEN, fmt, tp);		/* format it	*/
	fputs(result, stdout);
}
