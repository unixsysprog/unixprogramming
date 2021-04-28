/* builtin.c
 * contains the switch and the functions for builtin commands
 */

#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include        <unistd.h>
#include        <errno.h>
#include        <stdlib.h>
#include        <limits.h>
#include	"smsh.h"
#include	"varlib.h"
#include        "flexstr.h"

int assign(char *);
int okname(char *);
static int  change_dir(char *);
static void do_exit(char *);
static void do_read(char *);

int builtin_command(char **args, int *resultp)
/*
 * purpose: run a builtin command 
 * returns: 1 if args[0] is builtin, 0 if not
 * details: test args[0] against all known builtins.  Call functions
 */
{
	int rv = 0;

	if (strcmp(args[0],"set") == 0){	  /* 'set' command? */
	   VLlist();
	   *resultp = 0;
	   rv = 1;
	}
	else if (strchr(args[0], '=') != NULL){   /* assignment cmd */
	  *resultp = assign(args[0]);
	  if (*resultp != -1)		    /* x-y=123 not ok */
	    rv = 1;
	}
	else if (strcmp(args[0], "export") == 0){
	  if (args[1] != NULL && okname(args[1]))
	    *resultp = VLexport(args[1]);
	  else
	    *resultp = 1;
	  rv = 1;
	}
	else if (strcmp(args[0], "cd") == 0){     /* cd command */
	  *resultp = change_dir(args[1]);
	   rv = 1;      	     
	}
	else if (strcmp(args[0], "exit") == 0){   /*exit command */
	  do_exit(args[1]);
	  rv = 1;
	}
	else if (strcmp(args[0],"read") == 0){    /* read command */
	  do_read(args[1]);
	  rv = 1;	  
	}
	return rv;
}

int assign(char *str)
/*
 * purpose: execute name=val AND ensure that name is legal
 * returns: -1 for illegal lval, or result of VLstore 
 * warning: modifies the string, but retores it to normal
 */
{
	char	*cp;
	int	rv ;

	cp = strchr(str,'=');
	*cp = '\0';
	rv = ( okname(str) ? VLstore(str,cp+1) : -1 );
	*cp = '=';
	return rv;
}
int okname(char *str)
/*
 * purpose: determines if a string is a legal variable name
 * returns: 0 for no, 1 for yes
 */
{
	char	*cp;

	for(cp = str; *cp; cp++ ){
		if ( (isdigit((int)*cp) && cp==str) || !(isalnum((int)*cp) || *cp=='_' ))
			return 0;
	}
	return ( cp != str );	/* no empty strings, either */
 }

int oknamechar(char c, int pos)
/*
 * purpose: used to determine if a char is ok for a name
 */
{
	return ( (isalpha((int)c) || (c=='_' ) ) || ( pos>0 && isdigit((int)c) ) );
}


/* change_dir -
         executes chdir for the path passed 
	 returns 0 if chdir is successful
	 else returns  -1
*/
int change_dir(char* path)
{
    if (path == NULL || path[0] == '\0')
        path = VLlookup("HOME");

    if (chdir(path) == -1){
      fprintf(stderr,"smsh: %s\n",strerror(errno));
      return -1;
    }

    return 0;
}

/*
  do_exit -
          calls system _exit()
	  if a number is passed then convert string to long using strtol
	  and pass it to _exit();
*/
void do_exit(char* str)
{     
    char *endptr;
    long status = 0;
     
    if (str != NULL){
      status = strtol(str,&endptr,10);
    }

    _exit(status);
}

/*
  do_read - 
         reads the line character by character till it find a new line 
	 character and stores into FLEXSTR object
	 put the FLEXSTR string value for the variable passed with read
	 into variable store.
 */
void do_read(char* arg)
{
     char* val = NULL;
     //FLEXSTR fs;
     //char cp;

     if (arg == NULL)
       return;

     if ((val = next_cmd(NULL,stdin))!= NULL)
       VLstore(arg,val);

     /*fs_init(&fs,LINE_MAX);
     cp = fgetc(stdin);
     while (cp != '\0' && cp != '\n'){
       fs_addch(&fs,cp);
       cp = fgetc(stdin);
     }
     val = fs_getstr(&fs);
     if (val != NULL)
        VLstore(arg,val);
     fs_free(&fs);     */
}
