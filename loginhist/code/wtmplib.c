
#include "wtmplib.h"

static  char    wtmpbuf[NRECS * UTSIZE];                /* storage      */
static  int     num_recs ;                              /* num stored   */
static  int     cur_rec  = -1;                          /* next to go   */
static  int     fd_wtmp  = -1;                          /* read from    */
static  int     rseek_pos = -1;                         /* reverse seek position */

static  int     wtmp_reload();                          /* reads file and loads to buffer */
static  int     wtmp_rseek();                        /* position the pointer to right location when reverse seek*/

int wtmp_open( char *filename )
{
        fd_wtmp = open( filename, O_RDONLY );           /* open it      */
        rseek_pos = lseek(fd_wtmp,0,SEEK_END);           /* seek to end of file */
        //printf("%d\n", seek_pos);
        num_recs = -1;                                   /* no recs yet  */
        return fd_wtmp;                                 /* report       */
}

struct utmp *wtmp_readback()
{
        struct utmp *recp;
	int ret = -1;

        if ( fd_wtmp == -1 )                            /* error ?      */
                return NULLUT;

        if ( cur_rec == -1 ){

	  ret =  wtmp_rseek();
          
	  if ( ret == -1 || wtmp_reload() <= 0 )              /* any more ?   */
	    return NULLUT;
	  
          cur_rec = num_recs - 1;
	}
                                        /* get address of previous record    */
        recp = (struct utmp *) &wtmpbuf[cur_rec * UTSIZE];

        cur_rec--;
        return recp;
}

struct utmp *wtmp_readnext()
{
        struct utmp *recp;

        if ( fd_wtmp == -1 )                            /* error ?      */
                return NULLUT;

        if ( cur_rec == num_recs && wtmp_reload()==0 )    /* any more ?   */
                return NULLUT;
                                        /* get address of next record    */
        recp = (struct utmp *) &wtmpbuf[cur_rec * UTSIZE];

        cur_rec++;
        return recp;
}

static int wtmp_reload()
/*
 *      read next bunch of records into buffer
 */
{
        int     amt_read;
        //printf("reloading from file \n");
  
       
        amt_read = read(fd_wtmp, wtmpbuf, NRECS*UTSIZE);   /* read data	*/
	if ( amt_read < 0 )			/* mark errors as EOF   */
	     amt_read = -1;
                                      
        num_recs = amt_read/UTSIZE;		/* how many did we get?	*/
        cur_rec  = 0;	       	/* set  pointer to begin of last record	*/

        return num_recs;			/* report results	*/

}

int wtmp_rseek()
{
    if (rseek_pos == 0)return -1;

    rseek_pos = rseek_pos - NRECS*UTSIZE;
  
    if ( rseek_pos < 0 )
         rseek_pos = 0;

    if ( lseek( fd_wtmp,rseek_pos, SEEK_SET)  == -1){

      fprintf(stderr,"seek failed");
      return -1;
    }
   
    return 0;

}

int wtmp_close()
{
	int rv = 0;
        if ( fd_wtmp != -1 )                    /* don't close if not   */
                rv = close( fd_wtmp );          /* open                 */
	return rv;
}
