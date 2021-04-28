#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include	"smsh.h"
#include	"splitline.h"
#include	"varlib.h"

/**
 **	small-shell version 4.2
 **		first really useful version after prompting shell
 **		this one parses the command line into strings
 **		uses fork, exec, wait, and ignores signals
 **		It also uses the flexstr library to handle
 **		a the command line and list of args
 **/

#define	DFL_PROMPT	"> "
#define DFL_COMMENT     '#'

static FILE* read_args(int , char** ,int *);
static void setup();


int main(int ac, char** av)
{
        char	*cmdline, **arglist;
	int	result;
	int     is_stdin = -1;
        int     process(char** );
	FILE    *f ;
	char    *prompt = NULL;
	char    *line;

	if ((f = read_args(ac, av,&is_stdin))== NULL){
	  fprintf(stderr,"smsh: cannot open file\n");
	  exit(1);
	}

	if (is_stdin == 1)
	  prompt = DFL_PROMPT;

	setup();

	while ( (line = next_cmd(prompt, f)) != NULL )
	{
	  if (line[0] == DFL_COMMENT)
	    continue;
	  cmdline = substitute_var(line);

	  if ( (arglist = splitline(cmdline)) != NULL  ){
	    result = process(arglist);
	    freelist(arglist);
	  }
	  free(line);
	  free(cmdline);
	}
	
	is_valid_close();                         /* checks if list for "if" is empty*/

	if (is_stdin == 0)
	  fclose(f);
	

	return 0;
}

/*
  read_args - 
              @nargs - number of command line arguments passed
	      @argv - arguments passed
	      @isstdin - pointer to determine it is a stdin or script file.
	      if nargs >2 then prints error and exits
              reads the next argument passed which is a script file
	      does fopen on the passed in argument and assign to FILE*
	      if no argumnet is passed then the FILE* is stdin
	      returns FILE* 
	      if fopen fails then returns NULL pointer.
           
 */
FILE* read_args(int nargs, char** argv, int* isstdin)
{
        FILE  *fp = NULL;
    	if (nargs > 2){
	  fprintf(stderr,"smsh : too many arguments.\n");
	  exit(1);
	}

	if (nargs == 2){
	  if ((fp = fopen(argv[1],"r"))== NULL){
	    fprintf(stderr,"smsh : cannot open script file %s\n",argv[1]);
	    exit(1);
	  }
	  *isstdin = 0;
	}
	else{
	  fp = stdin; 
	  *isstdin = 1;
	}
	return fp;

}
void setup()
/*
 * purpose: initialize shell
 * returns: nothing. calls fatal() if trouble
 */
{
	extern char **environ;

	VLenviron2table(environ);
	signal(SIGINT,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

}

void fatal(char *s1, char *s2, int n)
{
	fprintf(stderr,"Error: %s,%s\n", s1, s2);
	exit(n);
}
