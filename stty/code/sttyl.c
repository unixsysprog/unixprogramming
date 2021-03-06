#include        <stdio.h>
#include        <fcntl.h>
#include        <sys/ioctl.h>
#include        <errno.h>
#include        <ctype.h>
#include        <stdlib.h>
#include        <string.h>
#include        <termios.h>
#include        "flags.h"
/**
 **     sttyl - program to set and display termios settings
 **/

#define ASCII_DELETE      127
#define _POSIX_VDISABLE    0

/* setting arguments functions*/
static void process_arguments(const int , int  , char** , struct termios* );
static int  find_controlsymbol(const char* , int*);
static int  set_controlsymbol(const int,const char*  , struct termios* );
static int  find_set_attribute(const struct flaginfo* t, const char* , tcflag_t *);

/* display functions*/
static void show_default(const int ,const struct termios* info);
static void show_baud(speed_t );
static void show_rows_columns(const int );
static void show_control_chars(const struct termios* info);
static void show_terminal_flags(const struct termios *);
static void show_flagset(const int value,const struct flaginfo bitnames[] );

int main(int ac, char* av[])
{
    int fd;
    struct termios ttyinfo;

    fd = open( "/dev/tty", O_RDONLY);           /*open the terminal */

    if (fd == -1) {
        fprintf(stderr,"sttyl: %s\n", strerror(errno));
        exit(1);
    }

    if (tcgetattr(fd, &ttyinfo) == -1) {         /* get terminal sttributes*/
        fprintf(stderr,"sttyl: %s\n", strerror(errno));
        exit(1);
    }

    if (ac == 1) {                               /* if no arguments passed display current setting*/
        show_default(fd ,&ttyinfo);
    }
    else {                                       /* process arguments*/  
        process_arguments(fd, ac, av, &ttyinfo);
    }
    return 0;
}


/*
  process_arguments - processes the command line arguments and finds
                      if it is a valid one by checking in all the tables
		      @fd - file descriptor for terminal
		      @nargs - number of arguments
		      @argv - array that holds arguments
		      @info - struct termios that hold settings of terminal
 */
void process_arguments(const int fd, int nargs, char** argv, struct termios* info)
{
    int cur_arg = 1;
    int isvalid_arg = 0;                          /*holds the return value for check argument in table*/
    int index = 0;                                /* holds the position in c_ccflags table*/
    char* cur_flag = NULL;

    if (fd == -1 || info == NULL){
      return;
    }
    while ( cur_arg < nargs) {
        isvalid_arg = 0;
        cur_flag = argv[cur_arg];

                                                  /*check if it is a control chracter setting*/
        if (find_controlsymbol(cur_flag,&index) && index != -1) {
            cur_arg++;
            cur_flag = argv[cur_arg];
                                                  /* it is control characer set the value */
            if (set_controlsymbol(index,cur_flag,info)) {
                cur_arg++;
                continue;
            }
            else {
                fprintf(stderr,"sttyl: invalid argument to `%s'\n",c_ccflags[index].fl_name);
                exit(1);
            }
        }

        if (isvalid_arg == 0)                     /* find the passed argument in input_flags(c_iflag)table*/
            isvalid_arg = find_set_attribute(input_flags, cur_flag, &info->c_iflag);
        if (isvalid_arg == 0)                     /* not found in input_flags then check in output_flags(c_oflag) table*/
            isvalid_arg = find_set_attribute(output_flags, cur_flag, &info->c_oflag);
        if (isvalid_arg == 0)                     /* not found in above tables then check in control_flags table*/
            isvalid_arg = find_set_attribute(control_flags, cur_flag, &info->c_cflag);
        if (isvalid_arg == 0)                     /* not found in above tables then check inlocal_flags (c_lflag)table*/
            isvalid_arg = find_set_attribute(local_flags, cur_flag, &info->c_lflag);

        if (isvalid_arg == 0) {                   /* all the tables are checked and not found anywhere*/
	  fprintf(stderr,"sttyl:  invalid argument `%s'\n", cur_flag);
          exit(1);
        }
        cur_arg++;
    }
    if (tcsetattr(fd,TCSANOW,info) == -1) {                 /* commit the attributes to terminal */
    	fprintf(stderr,"sttyl: `%s'\n", strerror(errno));
	exit(1);
    }
}


/*
  find_controlsymbol - find if the passed argument is in termios c_ccflag array
T                      if found set the pos of the array element
		       if found returns 1 else returns 0
		       @flag - element to search in the array
		       @pos - set this pointer to the index of the array if found
*/
int find_controlsymbol(const char* flag, int* pos)
{
    int i;
    *pos = -1;
    for (i =0; c_ccflags[i].fl_name != NULL; i++) {
                                                  /* check if flag is member*/
        if (strcmp(c_ccflags[i].fl_name, flag)==0) {
            *pos = i;
            return 1;
        }
    }
    return 0;
}


/*
  set_controlsymbol - set the control character to c_cc array and commit to driver
                      @ fd - file descriptor for terminal
		      @ index - position of control in the c_cc array to be changed
		      @ symbol- value to be set to the control character
		      returns 0 if not successful, 1 if success.
*/
int set_controlsymbol(const int index,const char* symbol, struct termios* info)
{

    if (index == -1 || !symbol ||  symbol[0] =='\0' )
        return 0;
    
    if (strlen(symbol) > 1)
      return 0; 

    info->c_cc[c_ccflags[index].fl_value] = symbol[0];
    return 1;
}


/*
  find_set_attribute - find if the passed argument is in the table
                       if found then enable/disable the attribute
		       if the flag is found in the passed in table return 1
		       else return 0
		       @table[] - table to search
		       @flag  - attribute to check in table
		       @tcflag - termios member flag to change if @flag is found in table
*/

int find_set_attribute(const struct flaginfo* table,const char* flag, tcflag_t* tcflag)
{
    int i;
    int disableattr = 0;                          /* holder if argument is passed with '-'*/

    if (flag[0] == '-') {                         /* check if the attribute needs to be diabled*/
        disableattr = 1;
        flag = flag+1;
    }

    for (i =0;  table[i].fl_name != NULL ; i++) {                                                 
        if (strcmp(table[i].fl_name, flag) == 0) {/* if the passed in flag is member of table*/
            if (disableattr == 1)                 /* if disabled then negate the attribute*/
                *tcflag &= ~table[i].fl_value;
            else
                *tcflag |= table[i].fl_value;

            return 1;
        }
    }
    return 0;
}


/*
  show_default - goes through the current termios settings
                 @ fd - file descriptor for terminal
		 @ info - struct that hold current termios settings
*/
void show_default(const int fd,const struct termios* info)
{
    if (info ==  NULL)
        return;

    show_baud(cfgetospeed(info));                 /* get and show baud rate */
    show_rows_columns(fd);                        /* get and show rows and columns */
    show_control_chars(info);                     /* get and show control characters */
    show_terminal_flags(info);                    /* get and show terminal flags */
}


/*
 *      prints the speed
 */
void show_baud(speed_t  speed )
{
    
    switch (speed) {
        case B50:       printf("speed 50 baud;");         break;
        case B75:       printf("speed 75 baud;");         break;
        case B110:      printf("speed 110 baud;");        break;
        case B134:      printf("speed 134.5 baud;");      break;
        case B150:      printf("speed 150 baud;");        break;
        case B200:      printf("speed 200 baud;");        break;
        case B300:      printf("speed 300 baud;");        break;
        case B600:      printf("speed 600 baud;");        break;
        case B1200:     printf("speed 1200 baud;");       break;
        case B1800:     printf("speed 1800 baud;");       break;
        case B2400:     printf("speed 2400 baud;");       break;
        case B4800:     printf("speed 4800 baud;");       break;
        case B9600:     printf("speed 9600 baud;");       break;
        case B19200:    printf("speed 19200 baud;");      break;
        case B38400:    printf("speed 38400 baud;");      break;
        default:        printf("speed not identified;");  break;

    }
}


/*
  show_rows_columns - uses ioctl() to get the size of terminal
                      if TIOCWINSZ is defined else get environment variables.
 */
void show_rows_columns(const int fd)
{
    char *env_rows =NULL;
    char *env_cols = NULL;
    int   nrows, ncols;

    if (fd == -1)
        return;

#ifdef TIOCGWINSZ
    struct winsize wininfo;

    if (ioctl(fd, TIOCGWINSZ, &wininfo) == -1 ) {
        fprintf(stderr,"sttyl: %s\n", strerror(errno));
        exit(1);
    }

    printf(" rows %u; cols %u;\n", wininfo.ws_row, wininfo.ws_col);
    return;
#endif

    env_rows = getenv("LINES");
    env_cols = getenv("COLUMNS");
    if (env_rows != NULL && (nrows = atoi(env_rows)) >= 0)
      printf(" rows %d;", nrows);

    if (env_cols != NULL && (ncols = atoi(env_cols)) >= 0)
      printf(" cols %d\n",ncols);
}


/*
  show_control_chars - displays the control characters in c_cc array
                       of current termios struct
		       if the character is one character just displays the character
		       if the character is Ctrl -x displays ^ and
		       converts character relative to 'A' i.e. 'A' +ch -1 if ch is character
 */
void show_control_chars(const struct termios* info)
{
    int i;
    char ch;                                      /* local variable to hold control char*/
    if (info == NULL)
        return;

    for (i=0; c_ccflags[i].fl_name != NULL; i++) {
        ch = info->c_cc[c_ccflags[i].fl_value];
        if (isprint(ch)) {                        /* check if it is single character*/
            printf("%s = %c;", c_ccflags[i].fl_name, ch);
        }
        else if (ch == ASCII_DELETE) {            /* check if it is delete key*/
            printf("%s = ^?;", c_ccflags[i].fl_name);
        }
        else if (ch == _POSIX_VDISABLE) {         /* check if the control action is defined*/
            printf("%s = <undef>;", c_ccflags[i].fl_name);
        }
        else {                                    /* if the control is two character */
            printf("%s = ^%c;", c_ccflags[i].fl_name, 'A' + ch -1 );
        }
    }

}


/*
 *  show the values of the flag sets_: c_iflag , c_lflag,
 *  c_oflag and c_cflag.
 */
void show_terminal_flags(const struct termios *info )
{
    if (info == NULL)
        return;

    printf("\n");
    show_flagset(info->c_iflag, input_flags);

    printf("\n");
    show_flagset(info->c_lflag, local_flags);

    printf("\n");
    show_flagset(info->c_oflag, output_flags);

    printf("\n");
    show_flagset(info->c_cflag, control_flags);
    printf("\n");
}


/*
 * show_flagset -
                 @ value - value of the flag
		 @ bitnames - corresponding table for the flag
		 check each bit pattern and display descriptive title
		 and displays '-attribute display name' if the attribute is off
		 or 'attribute display name' if the attribute is on.
 */
void show_flagset(const int value,const struct flaginfo bitnames[] )
{
    int     i;

    for ( i=0; bitnames[i].fl_name != NULL ; i++ ) {
        if ( value & bitnames[i].fl_value )
            printf("%s;",bitnames[i].fl_name);
        else
            printf("-%s;",bitnames[i].fl_name);
    }
}
