/* splitline.c - commmand reading and parsing functions for smsh
 *    
 *    char *next_cmd(char *prompt, FILE *fp) - get next command
 *    char **splitline(char *str);           - parse a string
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include        <ctype.h>
#include	"splitline.h"
#include	"smsh.h"
#include        "varlib.h"
#include	"flexstr.h"

#define  DOLLAR_CHAR  '$'
#define  ESCAPE_CHAR  '\\'
#define  COMMENT_CHAR '#'


/*
 * purpose: read next command line from fp
 * returns: dynamically allocated string holding command line
 *  errors: NULL at EOF (not really an error)
 *          calls fatal from emalloc()
 *   notes: allocates space in BUFSIZ chunks.  
 */
char * next_cmd(char *prompt, FILE *fp)
{
	int	c;				/* input char		*/
	FLEXSTR	s;				/* the command		*/
	int	pos = 0;

	fs_init(&s, 0);				/* initialize the str	*/

	if (prompt != NULL)
	  printf("%s", prompt);			/* prompt user if stdin	*/
	
	while ((c = fgetc(fp)) != EOF) 
	{
	  /* end of command? */
	  if (c == '\n' )
	    break;

	  /* no, add to buffer */
	  fs_addch(&s, c);
	  pos++;
	  
	}
	if (c == EOF && pos == 0)	/* EOF and no input	*/
	  return NULL;			/* say so		*/

	fs_addch(&s, '\0');		/* terminate string	*/
	return fs_getstr(&s);
}

/**
 **	splitline ( parse a line into an array of strings )
 **/
#define	is_delim(x) ((x)==' '||(x)=='\t')

char ** splitline(char *line)
/*
 * purpose: split a line into array of white-space separated tokens
 * returns: a NULL-terminated array of pointers to copies of the tokens
 *          or NULL if line is NULL.
 *          (If no tokens on the line, then the array returned by splitline
 *           contains only the terminating NULL.)
 *  action: traverse the array, locate strings, make copies
 *    note: strtok() could work, but we may want to add quotes later
 */
{
	char	*newstr();
	char	*cp = line;			/* pos in string	*/
	char	*start;
	int	len;
	FLEXLIST strings;

	if ( line == NULL )			/* handle special case	*/
	  return NULL;

	fl_init(&strings,0);

	while(*cp != '\0')
	{
	  while ( is_delim(*cp) )		/* skip leading spaces	*/
	    cp++;
	  
	  if (*cp == '\0' )		/* end of string? 	*/
	    break;			/* yes, get out		*/

	  /* mark start, then find end of word */
	  start = cp;
	  len   = 1;
	  while (*++cp != '\0' && !(is_delim(*cp)) )
	    len++;
	  fl_append(&strings, newstr(start, len));
	}
	fl_append(&strings, NULL);
	return fl_getlist(&strings);
}

/*
 * purpose: constructor for strings
 * returns: a string, never NULL
 */
char *newstr(char *s, int l)
{
	char *rv = emalloc(l+1);

	rv[l] = '\0';
	strncpy(rv, s, l);
	return rv;
}

void 
freelist(char **list)
/*
 * purpose: free the list returned by splitline
 * returns: nothing
 *  action: free all strings in list and then free the list
 */
{
	char	**cp = list;
	while( *cp )
	  free(*cp++);
	free(list);
}

void * emalloc(size_t n)
{
	void *rv ;
	if ( (rv = malloc(n)) == NULL )
	  fatal("out of memory","",1);
	return rv;
}
void * erealloc(void *p, size_t n)
{
	void *rv;
	if ( (rv = realloc(p,n)) == NULL )
	  fatal("realloc() failed","",1);
	return rv;
}

/*
  substitute_var -
                 @line - line to processed for variable substitution
		 read character by character and if a '$' is found
		 and previous char is not '\\' then it
		 calls find_replace_var to do variable substitution
		 once replacement is done, processing is started
		 from place where variable has ended.
		 if previous char is '\\' then character is taken literally.
 */
char* substitute_var(char* line)
{
        char *c = line;
	FLEXSTR s;
	int len, pos = 0; 
	int escape = 0;            /* variable to hold if last char is '\\'*/
	int find_replace_var(char* ,int* ,FLEXSTR*);
	
	if (line == NULL)
	    return NULL;

	len = strlen(line);
	if (len == 0)
	    return '\0';

	fs_init(&s ,0);

	while (*c != '\0' && pos < len){
	  if (*c == DOLLAR_CHAR &&   /* '$' found and not follows '\\'*/
	         escape == 0 &&
	         find_replace_var(line, &pos, &s) != -1){
	         c = line + pos;
	         escape = 0;
	         continue;
	      }
	                             /* backslash found*/
	      if (*c == ESCAPE_CHAR && escape == 0){
	         escape = 1;
	      }
	      else{
	         fs_addch(&s, *c);
	         escape = 0;
	      }

	      c++;
	      pos++;
	}
	return fs_getstr(&s);
}

/*
  find_replace_var - 
                   @line - current line that is processes
		   @pos  - pos where the '$' is found
		           passed as pointer for calling function to 
			   know where to start resume procesing
		   @s    - FLEXSTR object which is being modified for variable
		           substitution

		   if next character after '$' is another '$' or a digit 
		   or end of string then we return.
		   else look at one character at a time and if it is a valid
		   add the character to FLEXSTR object till an invalid char
		   is found. once the variable is obtained, do VLlookup 
		   to get the value. If varaible is found then add the value
		   to passed in FLEXSTR s.This way the variable is substitute
		   with value. if VLlookup fails to find variable then
		   just ignore the variable (i.e. substitute with '' in place
		   of variable.
		   
		   
 */
int find_replace_var(char* line, int* pos, FLEXSTR *s)
{
       FLEXSTR p;
       char* var = NULL;
       char* value = NULL;
       int ret = -1;
       int index = *pos + 1;
       char* cp = line + index;
       
       /* if next character is '$' or a digit return, no substitution needed*/
       if (*cp == '\0' || *cp == DOLLAR_CHAR || isdigit((int)*cp))
	 return ret;

       fs_init(&p,0);

       while (*cp !='\0'){
	    if (isdigit((int)*cp) || isalnum((int)*cp) || *cp == '_' ){
	    fs_addch(&p,*cp);
	    cp++;
	    index++;
	 }
	 else
	    break;
       }

       var   = fs_getstr(&p);
       value = VLlookup(var);  /*lookup in setup for the variable value*/
 
       if (value != NULL){     /* if variable is found then substitute the value*/
	 fs_addstr(s,value);
	 *pos = index;
	 ret = 0;
       }
       fs_free(&p);           /* release the flexstr created for substitution*/
       return ret;
}
