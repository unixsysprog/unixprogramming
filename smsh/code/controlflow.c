/* controlflow.c
 *
 * "if" processing is done with two state variables
 *    if_state and if_result
 */
#include	<stdio.h>
#include        <stdlib.h>
#include	<string.h>
#include        "limits.h"
#include        "dllist.h"
#include	"smsh.h"


enum states   { NEUTRAL, WANT_THEN, THEN_BLOCK, ELSE_BLOCK };
enum results  { SUCCESS, FAIL, NEVER };
enum keywords { NONE, K_IF, K_THEN, K_ELSE, K_FI };
struct kw { char *str; int tok; };

struct kw words[] = { {"if", K_IF}, {"then", K_THEN},{"else", K_ELSE},
                      {"fi", K_FI}, {NULL,0} };



int  process_control_cmd(int  ,char*  ,char** );
int  is_valid_state(int );
void execute_if(char** , int* );
int  is_a_control_word(char *s);
int  syn_err(char *);



/*
 * purpose: determine the shell should execute a command
 * returns: 1 for yes, 0 for no
 * details: if in THEN_BLOCK and if_result was SUCCESS then yes
 *          if in ELSE_BLOCK and if_result was FAIL    then yes
 *          if in WANT_THEN  then syntax error (sh is different)
 */
int ok_to_execute()
{
    int	rv = 1;		/* default is positive */
    int state, result;
   
    if (is_list_empty() == 1)
      return rv;

    state  = get_list_state();
    result = get_list_result();

    if (state == WANT_THEN){
        syn_err("then expected");
        rv = 0;
    }
    else if (state == THEN_BLOCK)
        rv = (result == SUCCESS ? rv : 0);
    else if (state == ELSE_BLOCK)
        rv = (result == FAIL ? rv : 0);

    return rv;
}

/*
 * purpose: boolean to report if the command is a shell control command
 * returns: 0 or 1
 */
int is_control_command(char *s)
{
    return ( is_a_control_word(s) != NONE );
}

int is_a_control_word(char *s)
{
    int	i;

    for(i=0; words[i].str != NULL ; i++ )
      {
	if (strcmp(s, words[i].str) == 0)
		return words[i].tok;
      }
    return NONE;
}

/*
 * purpose: Process "if", "then", "else", "fi" - change state or detect error
 * returns: 0 if ok, -1 for syntax error
 */
int do_control_command(char **args)
{
    char *cmd = args[0];
    int	 rv = -1;

    if (strcmp(cmd,"if")==0){
      rv = process_control_cmd(K_IF, cmd, args);	        
    }
    else if (strcmp(cmd,"then")==0){
      rv = process_control_cmd(K_THEN, cmd,args);
    }
    else if (strcmp(cmd,"else") == 0){
      rv = process_control_cmd(K_ELSE, cmd, args); 
    }
    else if (strcmp(cmd,"fi")==0){
      rv = process_control_cmd(K_FI,cmd, args);
    }
    else 
      fatal("internal error processing:", cmd, 2);
    return rv;
}


int check_if_state()
{
    if (is_list_empty() == 0 && get_list_state() != NEUTRAL){
      syn_err("unexpected close of if block");
      return -1;
    }
    return 0;
 }


/*
  process_control_cmd -
                     @key - integer value of control cmd  K_IF,_THEN,K_ELSE,K_FI
		     @args - arguments after the control cmd
		     if it is a "if" command then execute the subsequent args
		     get the result of the execution
		     for "if","then","else" add the commands, result and state
		     to list that is maintained. 
		     for "fi" is found remove the members of list till the
		     corresponding "if"  member is found
		     in each step validity of passed in cmd is checked.
		     if no syntax error is found then returns 0 else -1
		     
 */
int process_control_cmd(int key, char* cmd, char** args)
{
      int rv = -1;
      int result = 0;
      char errmsg[LINE_MAX] ="";
      int isvalid = is_valid_state(key);
      
      if (isvalid == -1) {
	sprintf (errmsg, "%s unexpected", cmd);
	rv = syn_err(errmsg);
	return rv;
      }
     
      switch (key){
      case K_IF:
	execute_if(args+1, &result);
	add_item_list(K_IF, result,WANT_THEN);	  
	rv = 0;
	break;
      case K_THEN:
	result = get_list_result();
	add_item_list(K_THEN,result,THEN_BLOCK);
	rv = 0;
	break;
      case K_ELSE:
	result = get_list_result();
	add_item_list(K_ELSE,result,ELSE_BLOCK);
	rv = 0;
	break;
      case K_FI:
	remove_item_list();
	rv = 0;
	break;
      default:
	break;
      }
      return rv ;
}

/*
  execute_if - 
              @argv - control statement after "if"
	      @result - pointer to "result" of processing the if statement
	      
	      if it is not ok_to_execute that block of if
	      result is set to "NEVER" so that whole nested if
	      is skipped.
	      else process it using argv and get the result of the process.
*/
void execute_if(char** argv,int *result)
{
    int ret = 0;
    if (!ok_to_execute()) {
      *result = NEVER;
    }
    else {
      ret = process(argv);
      *result = (ret == 0 ? SUCCESS : FAIL);
    }
}


/* 
   is_valid_state -
               every IF should be followed by THEN , then optional ELSE
	       and IF block should end with FI
	       A list is maintained and the last state and command is 
	       the tail of list. based on what is last command , the above
	       sequence should follow or else it is not a valid command
	       returns -1 if the entered command is not valid one
	       else returns 0
*/
int is_valid_state(int key )
{
    int rv = 0;
    int isempty = is_list_empty();
    int state   = get_list_state();

    switch (key){
    case K_IF:
      if (isempty == 0 && (state == WANT_THEN || state == NEUTRAL))
	rv = -1;
      break;
    case K_THEN:
      if (isempty == 1 || state != WANT_THEN)
	rv = -1;
      break;
    case K_ELSE:
      if (isempty == 1 || state != THEN_BLOCK)
	rv = -1;
      break;
    case K_FI:
      if (isempty || state == WANT_THEN || state == NEUTRAL)
	rv = -1;
      break;
    default:
      break;
    }
    return rv;

}

/* purpose: handles syntax errors in control structures
 * details: resets state to NEUTRAL
 * returns: -1 in interactive mode. Should call fatal in scripts
 */
int syn_err(char *msg)
{
    fprintf(stderr,"syntax error: %s\n", msg);
    return -1;
}
