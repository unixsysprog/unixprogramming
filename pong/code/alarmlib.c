#include    <sys/time.h>
#include    <signal.h>
#include    <errno.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>

/*
 *	alarmlib.c
 *			timer functions for higher resolution clock
 *
 *			set_ticker( number_of_milliseconds )
 *				arranges for the interval timer to issue
 *				SIGALRM's at regular intervals
 */

int set_ticker( int n_msecs )
/*
 *	arg in milliseconds, converted into micro seoncds
 *	Returns -1 on error, 0 if no error
 */
{
    struct itimerval new_timeset, old_timeset;
    long    n_sec, n_usecs;

    n_sec = n_msecs / 1000 ;
    n_usecs = ( n_msecs % 1000 ) * 1000L ;

                                   /* set reload  */
    new_timeset.it_interval.tv_sec  = n_sec;
                                   /* new ticker value */
    new_timeset.it_interval.tv_usec = n_usecs;
                                   /* store this	*/
    new_timeset.it_value.tv_sec     = n_sec  ;
                                   /* and this 	*/
    new_timeset.it_value.tv_usec    = n_usecs ;

    if ( setitimer( ITIMER_REAL, &new_timeset, &old_timeset ) != 0 ) {
        printf("Error with timer..errno=%d\n", errno );
        return -1;
    }
    return 0;

}
