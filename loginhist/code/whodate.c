#include	<stdio.h>
#include	<sys/types.h>
#include	<utmp.h>
#include	<fcntl.h>
#include	<time.h>
#include	<stdlib.h>
#include        <string.h>
#include        "wtmplib.h"


#define	ARGDATE_FMT	"%Y %m %d"
#define	MAX_NAME	256
#define	MAXDATELEN	100
#define TIME_RANGE      86400			/*seconds in day*/

#define	DATE_FMT	"%Y-%m-%d %H:%M"	/* the default	*/


static time_t      t_spec   = 0;         /* stores the time specified */
static int         wtmp_fd  = -1;        /* file descriptor for wtmp file */
static size_t      wtmp_fsz = 0;         /* wtmp file size */


time_t      get_rectime ( );  		/* get time from buffer */
void        report_seekfail (int loc ); 


main(int ac, char **av)
{			
        char        file[MAX_NAME];
        char        cdate[MAXDATELEN];
	struct tm   t;
	
	void        read_wtmp   (char* ); 

        if ( ac < 4 || ac > 6){
	  fprintf(stderr,"incorrect arguments specified: %d", ac);
	  return 0;
        }

        if ( strcmp(av[1] ,"-f") == 0 ){

	   sprintf ( file,"%s",av[2]);
           sprintf (cdate,"%s %s %s",av[3],av[4],av[5]);
        }
        else {
	     
	    sprintf ( file,"%s", WTMP_FILE);
            sprintf (cdate,"%s %s %s",av[1],av[2],av[3]);
        }
                
        printf("%s\n", cdate);
	memset(&t, 0, sizeof(struct tm));

	if (strptime(cdate,ARGDATE_FMT, &t) == 0){
	   
	  fprintf(stderr,"cannot convert the date:%s\n",cdate);
	  exit(1);
	}

        t_spec  = mktime(&t);

        read_wtmp (file);

        return 0;
       

}

/*
	read_wtmp - opens , search, read and close wtmp file
*/
void read_wtmp(char* file)
{
	int             seek_pos;
	
	int             search_wtmp ( );	/* performs search on wtmp file */
	void            read_records( );	        /* reads the struct utmp record */

       if ( (wtmp_fd = wtmp_open(file)) == -1){
         
	 fprintf(stderr,"cannot open the file: %s\n",file);
	 exit(1);
       }
 
       if (  (wtmp_fsz  = lseek(wtmp_fd , 0, SEEK_END) )== -1){          /* get wtmp file size */
	
	 report_seekfail (0);
	 return;
       }
       
       if ( (seek_pos = search_wtmp( )) != -1){
	                                                                 /*set the position for actual reads*/
	 if ( lseek (wtmp_fd, seek_pos, SEEK_SET) == -1){	
	  
	   report_seekfail (seek_pos);
	   return;
	 }

	 read_records();
       }
       
       wtmp_close( );
}


/* 
*	search_wtmp - sets up for binary search and  
*		      malloc the struct sloc 
*	@return int - if the file does not have records for specified date returns -1 
*	              else returns number of records in the file branch	
*/
int search_wtmp( )
{
  int       bsearch_wtmp( int, int , int* );		/*recursion for binary search */
      
       int 	   seek_pos = -1;
       int*        bfind = malloc(sizeof(int));

       *bfind = 0;

       seek_pos = bsearch_wtmp( 0, wtmp_fsz , bfind);
       
       return seek_pos;
}

/*
*	read_records - reads the specific block records
*		       get the time of the record
*		       if the record time match with specified date
*		       display the record details	
*/
void read_records()
{
	double         diff = 0;
	struct utmp   *wtbufp;
	time_t         t2;

	void           show_info   ( struct utmp* wtmp );       /* show the data in specified format */


        while ( ( wtbufp = wtmp_readnext() ) != ((struct utmp *) NULL))  { 					
									   
          diff = difftime( (time_t)wtbufp->ut_time, t_spec);;
	  
          if ( wtbufp->ut_type == USER_PROCESS &&
	       diff >0 && diff < TIME_RANGE ) {

	    show_info( wtbufp );     				/* got the record. so show the data */
        
           }
	    
	   if (diff > TIME_RANGE)
		break;	

	}
}

/*
	bsearch_wtmp - recursively performs binary search 
*		       based on specified time range 
*       @param start - position in the file for the first record in the branch
*	@param end  - position in the file for the end of the branch
*	@param bfound- if record is found set this value to 1 in the calling branch and get the start
*                       or current seek location based on which branch the value. once this is set to 1.
*		       position is found and recursion just quits returning 
*                      same location. this pointer is there not to change the seek position when recursion returns.
*
*	gets the middle record in the branch and compares the time and recurses accordingly
*       recursion end condition is when current seek location is same as start or end position
*	or start and end match.
*	if the time specified is in the branch 
*/

int  bsearch_wtmp(int start , int end ,int* bfound)
{
       int    nblocks = 0;
       int    cur_seek = 0;
       int    ret      = -1;
       time_t t      = 0; 

       if (start >= end ) return ret;
      
       nblocks = (end - start )/UTSIZE;

       cur_seek =  start + (nblocks/2)*UTSIZE;

       if ( cur_seek == start || cur_seek == end)	/* if this condition succeeds then no records are found*/
           return ret;
       
       if ( lseek ( wtmp_fd, cur_seek, SEEK_SET) == -1){          
	 report_seekfail (0);
         return -1;
       }
	
       t =  get_rectime ();

       if (t >= t_spec  && t <= (t_spec + TIME_RANGE )  )
	 return cur_seek;		               /* found record close to the time specified */
       	       
       if ( t > t_spec  ){

	  ret = bsearch_wtmp( start, cur_seek, bfound);		/* perform search in top branch */

	  if (ret != -1 && bfound == 0){              /* got the branch , just set the seek position to start and return*/
	    ret = start;                                        
	    *bfound = 1;
	  }	 
       }
       else{
	 
	 ret = bsearch_wtmp (cur_seek, end, bfound);		/* perform search in bottom branch*/

	 if (ret != -1 && *bfound == 0){
	     ret = cur_seek;
	     *bfound = 1;
	  }
       }
       return ret;	
}


/*
	get_rectime() - gets time of the current record and returns it.
*/
time_t get_rectime()
{
      struct utmp  wtbuf;

       if (wtmp_fd == -1){
	 return 0;
       }
       if ( read (wtmp_fd, &wtbuf, UTSIZE)== -1){
          fprintf(stderr, "cannot read record");
	  return 0;
       }

       return (time_t)(wtbuf.ut_time);
          
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

	printf("%-16s", utbufp->ut_name);		/* the logname	*/
	printf(" ");					/* a space	*/
	printf("%-12.12s", utbufp->ut_line);		/* the tty	*/
	printf(" ");
	printf("%-6d ", utbufp->ut_pid );
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


void report_seekfail (int loc)
{
   fprintf(stderr,"seek failed at loc: %d", loc);
   wtmp_close();
   return;

}
