/* wtmplib.c  - functions to buffer reads from wtmp file 
 *
 *      functions are
 *              wtmp_open( filename )   - open file
 *                      returns -1 on error
 *              wtmp_next( )            - return pointer to next struct
 *                      returns NULL on eof
 *              wtmp_close()            - close file
 *
 *      reads NRECS per read and then doles them out from the buffer
 */
#include        <stdio.h>
#include        <fcntl.h>
#include        <sys/types.h>
#include        <utmp.h>
#include	<unistd.h>

#define NRECS   16
#define NULLUT  ((struct utmp *)NULL)
#define UTSIZE  (sizeof(struct utmp))

static  char    wtmpbuf[NRECS * UTSIZE];                /* storage      */
static  int     num_recs;                               /* num stored   */
static  int     cur_rec  = -1;                          /* next to go   */
static  int     fd_wtmp = -1;                           /* read from    */
static  int     seek_pos = -1;
static  int     wtmp_reload();
static  int     wtmp_seekpos();

int wtmp_open( char *filename )
{
        fd_wtmp = open( filename, O_RDONLY );           /* open it      */
        seek_pos = lseek(fd_wtmp,0,SEEK_END);           /* seek to end of file */
        printf("%d\n",sizeof(struct utmp));
        printf("%d\n", seek_pos);
        num_recs = 0;                                   /* no recs yet  */
        return fd_wtmp;                                 /* report       */
}

struct wtmp *wtmp_next()
{
        struct utmp *recp;

        if ( fd_wtmp == -1 )                            /* error ?      */
                return NULLUT;

        if ( cur_rec == -1 && wtmp_reload()==0 )    /* any more ?   */
                return NULLUT;
                                        /* get address of next record    */
        recp = (struct utmp *) &wtmpbuf[cur_rec * UTSIZE];

        cur_rec--;
        return recp;
}

static int wtmp_reload()
/*
 *      read next bunch of records into buffer
 */
{
        int     amt_read;
        printf("reloading from file \n");
  
         if ( wtmp_seekpos() == -1)
              return 0;

        amt_read = read(fd_wtmp, wtmpbuf, NRECS*UTSIZE);   /* read data	*/
	if ( amt_read < 0 )			/* mark errors as EOF   */
	     amt_read = -1;
                                      
        num_recs = amt_read/UTSIZE;		/* how many did we get?	*/
        cur_rec  = num_recs - 1;	       	/* set  pointer to begin of last record	*/

        return num_recs;			/* report results	*/

}

int wtmp_seekpos()
{
    if (seek_pos == 0)return -1;

    seek_pos = seek_pos - NRECS*UTSIZE;
  
    if ( seek_pos < 0 )
         seek_pos = 0;

    if ( lseek(fd_wtmp,seek_pos, SEEK_SET)  == -1){
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
