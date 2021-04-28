#include	<stdio.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include        <string.h>

#define	MAXNAME_SIZE	256

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

main(int ac, char *av[])
{			
	int		wtmpfd;			/* read from this descriptor */
        char            fname[MAXNAME_SIZE];
        char            uname[MAXNAME_SIZE];
	

        if ( ac < 2 || ac > 4){
            fatal("incorrect arguments specified.","");
        }
       
        if (ac == 2){
            strcpy(fname,WTMP_FILE);
            strcpy(uname,av[1]);
        }
        else if (ac == 4 && strcmp(av[1] ,"-f") == 0 ){
           strcpy(fname,av[2]);
           strcpy(uname,av[3]);
        }
        else{
            fatal("incorrect arguments specified.","");
        }

	if ( (wtmpfd = open(fname, O_RDONLY )) == -1 ){
                fatal("cannot open",fname);
	}
        
        read_wtmp(wtmpfd, uname);
       
	close( wtmpfd );

}

void read_wtmp(int fd, char* uname)
{
     struct utmp	utbuf;			/* read info into here */
     int             filesz;
     int             pos;

     if ( (filesz = lseek(wtmpfd,0,SEEK_END)) == -1 ){
                fatal("cannot seek " , WTMP_FILE);
        };
        
        pos = filesz - sizeof(utbuf);

        lseek(wtmpfd,pos,SEEK_SET);

	while ( read( wtmpfd, &utbuf, sizeof(utbuf) ) == sizeof(utbuf) ){
               
               if ( utbuf.ut_type == USER_PROCESS && 
                    strcmp (utbuf.ut_user, uname) == 0){

		   show_info( &utbuf );
                   break;
              }

              pos = pos - sizeof(utbuf);              
 
              if ( lseek(wtmpfd,pos, SEEK_SET) == -1 ){
                  fatal("cannot seek futher" , WTMP_FILE);              
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

#define	MAXDATELEN	100

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
