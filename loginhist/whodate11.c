#include	<stdio.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include        <string.h>

#define	MAXNAME_SIZE	256
#define	STRDATE_FMT	"%Y %m %d"
#define	MAXDATELEN	100

#define TEXTDATE
#ifndef	DATE_FMT
#ifdef	TEXTDATE
#define	DATE_FMT	"%b %e %H:%M"		/* text format	*/
#else
#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/
#endif
#endif


void fatal ( char *, char *);
void show_info (struct utmp* wtmp);
void convert_date (char* , struct tm*);
void read_wtmp(int , struct tm*);

main(int ac, char *av[])
{			
	int		wtmpfd;			/* read from this descriptor */
        char            fname[MAXNAME_SIZE];
        char            cdate[MAXNAME_SIZE];
	struct tm       tm;

        if ( ac < 4 || ac > 6){
            fatal("incorrect arguments specified.","");
        }
       
        if (ac == 4){
            strcpy(fname,WTMP_FILE);
            sprintf(cdate,"%s %s %s",av[1],av[2],av[3]);
        }
        else if (ac == 6 && strcmp(av[1] ,"-f") == 0 ){
           strcpy(fname,av[2]);
           sprintf(cdate,"%s %s %s",av[3],av[4],av[5]);
        }
        else{
            fatal("incorrect arguments specified.","");
        }
         
        printf("%s\n", cdate);

	if ( (wtmpfd = open(fname, O_RDONLY )) == -1 ){
                fatal("cannot open",fname);
	}
        
        memset(&tm, 0, sizeof(struct tm));
        convert_date(cdate, &tm);

        read_wtmp(wtmpfd , &tm);
       
	close( wtmpfd );

}

void convert_date( char* date, struct tm* t)
{
 
  char buf[255];

  strptime(date,STRDATE_FMT, t);

  strftime(buf,sizeof(buf),STRDATE_FMT,t);
  printf("%s\n", buf);
}

/* workhorse */
void read_wtmp(char* file, char* user)
{
        struct utmp	*wtbufp,	/* holds pointer to next rec	*/
			*wtmp_next();	/* returns pointer to next	*/
	void		show_info( struct utmp * );

	if ( wtmp_open( file ) == -1 ){
		fprintf(stderr,"cannot open %s\n", file);
		exit(1);
	}


	while ( ( wtbufp = wtmp_next() ) != ((struct utmp *) NULL) ) {
	  
        if ( wtbufp->ut_type == USER_PROCESS &&
             strcmp( wtbufp->ut_user,user)==0) { 
                                     
	        show_info(wtbufp);   
                break;                      /* got the record */  
                     
	   }

	}

	wtmp_close( );
        

}


void read_wtmp(int fd, struct tm* t)
{
     struct utmp	utbuf;			/* read info into here */

     long               diff;

     time_t tt = mktime(t);

     while ( read( fd, &utbuf, sizeof(utbuf) ) == sizeof(utbuf) ){

              time_t tt1 = (time_t)utbuf.ut_time;
              diff = difftime(tt1,tt);

              if ( utbuf.ut_type == USER_PROCESS &&
                   diff >0 && diff < 86400) {
		   show_info( &utbuf );
                  
              }
                   
      }

}

void fatal(char *s1, char *s2)
{
	fprintf(stderr,"error: %s%s\n", s1, s2);
	exit(1);
}

void show_info(struct utmp* wtbufp)
{
        void showtime(time_t, char *);

        printf("%-8s", wtbufp->ut_name);		/* the logname	*/
	printf(" ");					/* a space	*/
 	printf("%-12.12s", wtbufp->ut_line);		/* the tty	*/
	printf(" ");                                    /* a space	*/

        //printf("%12d", (long)wtbufp->ut_time);					
	showtime( wtbufp->ut_time , DATE_FMT);		/* display time	*/

	if ( wtbufp->ut_host[0] != '\0' )
		printf(" (%s)", wtbufp->ut_host);	/* the host	*/

	printf("\n");					/* newline	*/
}



void showtime( time_t timeval , char *fmt )
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
