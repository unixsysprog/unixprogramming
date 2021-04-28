/* wtmplib.h - header file for declarations in wtmplib.c */

int wtmp_open( char* );
struct utmp *wtmp_next();
int wtmp_close();
