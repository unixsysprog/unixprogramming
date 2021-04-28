#include	<stdio.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include        <string.h>
#include        "wtmplib.h"

#define	MAX_NAME	256
#define	ARGDATE_FMT	"%Y %m %d"
#define	MAXDATELEN	100

#define TEXTDATE
#ifndef	DATE_FMT
#ifdef	TEXTDATE
#define	DATE_FMT	"%b %e %H:%M"		/* text format	*/
#else
#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/
#endif
#endif

static struct tm   t_spec;


main(int ac, char **av)
{			
        char        file[MAX_NAME];
        char        cdate[MAXDATALEN];
        void        search_wtmp (char* );
	void        show_info (struct utmp* wtmp);

        if ( ac != 4 || ac != 6){
            fprintf(stderr,"incorrect arguments specified.");
        }

        if ( strcmp(av[1] ,"-f") == 0 ){

           strcpy(file,av[2]);
           sprintf(cdate,"%s %s %s",av[3],av[4],av[5]);
        }
        else {

            strcpy(file,WTMP_FILE);
            sprintf(cdate,"%s %s %s",av[1],av[2],av[3]);
        }
                
        printf("%s\n", cdate);
	memset(&t_spec, 0, sizeof(struct tm));

	if (strptime(cdate,ARGDATE_FMT, t_spec) == NULL){
	   
	  fprintf(stderr,"cannot convert the date:%s\n",cdate);
	  exit(1);
	}
       
        search_wtmp (file);

        return 0;
       

}

void search_wtmp(char* file)
{
       if (wtmp_open(file) == -1){
         
	 fprintf(stderr,"cannot open the file: %s\n",file);
	 exit(1);
       }

       time_t tt = mktime(&t_spec);
       
       while ( ( wtbufp = wtmp_next(tt) ) != ((struct utmp *) NULL) ) { 
                                         
	        show_info(wtbufp);             
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
	printf(" ");					/* a space	*/
	showtime( utbufp->ut_time, DATE_FMT );		/* display time	*/
	if ( utbufp->ut_host[0] != '\0' )
		printf(" (%s)", utbufp->ut_host);	/* the host	*/
	printf("\n");					/* newline	*/
}


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
