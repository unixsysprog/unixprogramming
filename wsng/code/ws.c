#include	<stdio.h>
#include	<stdlib.h>
#include	<strings.h>
#include	<string.h>
#include	<netdb.h>
#include	<errno.h>
#include	<unistd.h>
#include	<sys/types.h>
#include        <sys/wait.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<signal.h>
#include        <dirent.h>
#include        <time.h>
#include        <ctype.h>
#include        "flexstr.h"
#include        "webtime.h"
#include	"socklib.h"
#include        "table.h"

/*
 * ws.c - a web server
 *
 *    usage: ws [ -c configfilenmame ]
 * features: supports the GET, HEAD command only
 *           runs in the current directory
 *           forks a new child to handle each request
 *           needs many additional features  
 */


#define	PORTNUM	80
#define	SERVER_ROOT	"."
#define	CONFIG_FILE	"wsng.conf"
#define	VERSION		"1"
#define SERVER_NAME     "Wsng"

#define	MAX_RQ_LEN	4096
#define	LINELEN		1024
#define	PARAM_LEN	128
#define	VALUE_LEN	512
#define MAXDATELEN      100
#define MAXFILESIZE     32
#define INDEX_HTML      "index.html"
#define INDEX_CGI       "index.cgi"
#define REQ_METHOD      "REQUEST_METHOD"
#define QUERY_STR       "QUERY_STRING"
#define	DATE_FMT	"%Y-%m-%d %H:%M"

char	myhost[MAXHOSTNAMELEN];
int	myport;
char	*full_hostname();

#define	oops(m,x)	{ perror(m); exit(x); }


static int is_headcmd = 0;  /*identifies if the current cmd processed is HEAD*/

/*
 * prototypes
 */

int	startup(int, char *a[], char [], int *);
void	read_til_crnl(FILE *);
void	process_rq( char *, FILE *);
void	bad_request(FILE *);
void	cannot_do(FILE *fp);
void    do_403(char* item, FILE* fp);
void	do_404(char *item, FILE *fp);
void	do_cat(char *f, FILE *fpsock);
void	do_exec( char *prog, FILE *fp);
void	do_ls(char *dir, FILE *fp);
int	ends_in_cgi(char *f);
char 	*file_type(char *f);
void	header( FILE *fp, int code, char *msg, char *content_type );
int	isadir(char *f);
char	*modify_argument(char *arg, int len);
int	not_exist(char *f);
void	fatal(char *, char *);
void	handle_call(int);
int	read_request(FILE *, char *, int);
char	*readline(char *, int, FILE *);

static  int   is_parent_cur_directory(char* );
static  void  process_index_file(char* , char* , FILE* );
static  char* read_response_line(FILE* );
static  void  do_head_exec(char* ,FILE* );
static  void  set_cgi_arguments(char* );
static  int   get_dirlist_html(char* ,int* ,int*, FLEXSTR* );
static  void  get_file_info(struct stat* , char* , char* , char* );
static  void  add_htmltable_rows(FLEXSTR* , char* , char* );
static  int   has_indexfile(char* , char* );

/* helpers for signal handling*/
static  void  handle_child_exit(int );
static  void  block_sigchld(int );

int
main(int ac, char *av[])
{
	int 	sock, fd;
	struct sigaction sa;

	/* set up */
	sock = startup(ac, av, myhost, &myport);

	/* sign on */
	printf("wsng%s started.  host=%s port=%d\n", VERSION, myhost, myport);
	
	memset(&sa,0, sizeof(struct sigaction));
	sa.sa_handler = handle_child_exit;
	sa.sa_flags   = SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);
	
	/* main loop here */
	while(1)
        {
	  block_sigchld(1);                             /* block sigchld*/
		fd    = accept( sock, NULL, NULL );	/* take a call	*/
		if ( fd == -1)
			perror("accept");
		else
			handle_call(fd);		/* handle call	*/

		block_sigchld(0);                       /* unblock sigchld*/ 
	}
	
	return 0;
	/* never end */
}
 

/*
 * handle_call(fd) - serve the request arriving on fd
 * summary: fork, then get request, then process request
 *    rets: child exits with 1 for error, 0 for ok
 *    note: closes fd in parent
 */
void handle_call(int fd)
{
	int	pid = fork();
	FILE	*fpin, *fpout;
	char	request[MAX_RQ_LEN];

	if ( pid == -1 ){
		perror("fork");
		return;
	}

	/* child: buffer socket and talk with client */
	if ( pid == 0 )
	{
		fpin  = fdopen(fd, "r");
		fpout = fdopen(fd, "w");
		if ( fpin == NULL || fpout == NULL )
			exit(1);

		if ( read_request(fpin, request, MAX_RQ_LEN) == -1 )
			exit(1);
		printf("got a call: request = %s", request);

		process_rq(request, fpout);
		fflush(fpout);		/* send data to client	*/
		exit(0);		/* child is done	*/
					/* exit closes files	*/
	}
	/* parent: close fd and return to take next call	*/
	close(fd);
}

/*
 * read the http request into rq not to exceed rqlen
 * return -1 for error, 0 for success
 */
int read_request(FILE *fp, char rq[], int rqlen)
{
	/* null means EOF or error. Either way there is no request */
	if ( readline(rq, rqlen, fp) == NULL )
		return -1;
	read_til_crnl(fp);
	return 0;
}

void read_til_crnl(FILE *fp)
{
        char    buf[MAX_RQ_LEN];
        while( readline(buf,MAX_RQ_LEN,fp) != NULL 
			&& strcmp(buf,"\r\n") != 0 )
                ;
}

/*
 * readline -- read in a line from fp, stop at \n 
 *    args: buf - place to store line
 *          len - size of buffer
 *          fp  - input stream
 *    rets: NULL at EOF else the buffer
 *    note: will not overflow buffer, but will read until \n or EOF
 *          thus will lose data if line exceeds len-2 chars
 *    note: like fgets but will always read until \n even if it loses data
 */
char *readline(char *buf, int len, FILE *fp)
{
        int     space = len - 2;
        char    *cp = buf;
        int     c;

        while( ( c = getc(fp) ) != '\n' && c != EOF ){
                if ( space-- > 0 )
                        *cp++ = c;
        }
        if ( c == '\n' )
                *cp++ = c;
        *cp = '\0';
        return ( c == EOF && cp == buf ? NULL : buf );
}
/*
 * initialization function
 * 	1. process command line args
 *		handles -c configfile
 *	2. open config file
 *		read rootdir, port
 *	3. chdir to rootdir
 *	4. open a socket on port
 *	5. gets the hostname
 *	6. return the socket
 *       later, it might set up logfiles, check config files,
 *         arrange to handle signals
 *
 *  returns: socket as the return value
 *	     the host by writing it into host[]
 *	     the port by writing it into *portnump
 */
int startup(int ac, char *av[],char host[], int *portnump)
{
	int	sock;
	int	portnum     = PORTNUM;
	char	*configfile = CONFIG_FILE ;
	int	pos;
	void	process_config_file(char *, int *);

	for(pos=1;pos<ac;pos++){
		if ( strcmp(av[pos],"-c") == 0 ){
			if ( ++pos < ac )
				configfile = av[pos];
			else
				fatal("missing arg for -c",NULL);
		}
	}
	process_config_file(configfile, &portnum);
			
	sock = make_server_socket( portnum );
	if ( sock == -1 ) 
		oops("making socket",2);
	strcpy(myhost, full_hostname());
	*portnump = portnum;
	return sock;
}


/*
 * opens file or dies
 * reads file for lines with the format
 *   port ###
 *   server_root path
 * at the end, return the portnum by loading *portnump
 * and chdir to the rootdir
 */
void process_config_file(char *conf_file, int *portnump)
{
	FILE	*fp;
	char	rootdir[VALUE_LEN] = SERVER_ROOT;
	char	param[PARAM_LEN];
	char	value[VALUE_LEN];
	char    descr[VALUE_LEN] ="";
	int	port;
	int	read_param(FILE *, char *, int, char *, int, char* , int );

	/* open the file */
	if ((fp = fopen(conf_file,"r")) == NULL)
		fatal("Cannot open config file %s", conf_file);

	/* extract the settings */
	while (read_param(fp, param, PARAM_LEN, value, VALUE_LEN,descr,VALUE_LEN) != EOF)
	{
		if (strcasecmp(param,"server_root") == 0)
			strcpy(rootdir, value);
		if (strcasecmp(param,"port") == 0)
			port = atoi(value);
		if (strcasecmp(param,"type") == 0)//adds the type data to table*/
		        add_type_table(value,descr);
		
	}
	fclose(fp);
	print_table();
	/* act on the settings */
	if (chdir(rootdir) == -1)
		oops("cannot change to rootdir", 2);
	*portnump = port;
	return;
}




/*
 * read_param:
 *   purpose -- read next parameter setting line from fp
 *   details -- a param-setting line looks like  name value
 *		for example:  port 4444
 *     extra -- skip over lines that start with # and those
 *		that do not contain two strings
 *   returns -- EOF at eof and 1 on good data
 *
 */
int read_param(FILE *fp, char *name, int nlen, char* value, int vlen, char* descr, int dlen)
{
	char	line[LINELEN];
	int	c;
	char	fmt[100] ;
	int     nvars = -1;

	sprintf(fmt, "%%%ds%%%ds%%%ds", nlen, vlen,dlen);

	/* read in next line and if the line is too long, read until \n */
	while(fgets(line, LINELEN, fp) != NULL )
	{
		if (line[strlen(line)-1] != '\n' )
			while( (c = getc(fp)) != '\n' && c != EOF )
	       		;
		nvars = sscanf(line,fmt,name,value,descr);
		if ((nvars == 2 || nvars == 3) && *name != '#' )
		      return 1;
		
	}
	return EOF;
}
	


/* ------------------------------------------------------ *
   process_rq( char *rq, FILE *fpout)
   do what the request asks for and write reply to fp
   rq is HTTP command:  GET /foo/bar.html HTTP/1.0
   ------------------------------------------------------ */

void process_rq(char *rq, FILE *fp)
{
	char	  cmd[MAX_RQ_LEN], arg[MAX_RQ_LEN];
	char	  *item, *modify_argument();

	if (sscanf(rq, "%s%s", cmd, arg) != 2 ){
		bad_request(fp);
		return;
	}

	/* if cmd is "HEAD" set the static variable is_headcmd to 1*/
	if (strcmp(cmd,"HEAD") == 0)
	    is_headcmd = 1;

	item = modify_argument(arg, MAX_RQ_LEN);	
	set_cgi_arguments(item);

	if (strcmp(cmd,"GET") != 0 && is_headcmd != 1 )
		cannot_do(fp);
	else if (not_exist(item))
		do_404(item, fp);
	else if (isadir(item))
	        do_ls(item, fp );	
	else if (ends_in_cgi(item))
	        do_exec(item, fp);
	else
		do_cat(item, fp );

	/* sets back the static variable to 0 once the cmd is processed*/
        is_headcmd =  0; 
}


/*
 * modify_argument
 *  purpose: many roles
 *		security - remove all ".." components in paths
 *		cleaning - if arg is "/" convert to "."
 *              returns: pointer to modified string
 *     args: array containing arg and length of that array
 */

char *
modify_argument(char *arg, int len)
{
	char	*nexttoken;
	char	*copy = malloc(len);

	if ( copy == NULL )
		oops("memory error", 1);

	/* remove all ".." components from path */
	/* by tokeninzing on "/" and rebuilding */
	/* the string without the ".." items	*/

	*copy = '\0';

	nexttoken = strtok(arg, "/");
	while( nexttoken != NULL )
	{
		if ( strcmp(nexttoken,"..") != 0 )
		{
			if ( *copy )
				strcat(copy, "/");
			strcat(copy, nexttoken);
		}
		nexttoken = strtok(NULL, "/");
	}
	strcpy(arg, copy);
	free(copy);

	/* the array is now cleaned up */
	/* handle a special case       */

	if ( strcmp(arg,"") == 0 )
		strcpy(arg, ".");
	return arg;
}
/* ------------------------------------------------------ *
   the reply header thing: all functions need one
   if content_type is NULL then don't send content type
   ------------------------------------------------------ */

void
header( FILE *fp, int code, char *msg, char *content_type )
{
	fprintf(fp, "HTTP/1.0 %d %s\r\n", code, msg);
	fprintf(fp, "Date: %s\r\n",rfc822_time(time(NULL)));
	fprintf(fp, "Server: %s/%s\r\n",SERVER_NAME, VERSION);
	if ( content_type )
		fprintf(fp, "Content-type: %s\r\n", content_type );
}

/* ------------------------------------------------------ *
   simple functions first:
	bad_request(fp)     bad request syntax
        cannot_do(fp)       unimplemented HTTP command
    and do_404(item,fp)     no such object
   ------------------------------------------------------ */

void
bad_request(FILE *fp)
{
	header(fp, 400, "Bad Request", "text/plain");
	fprintf(fp, "\r\nI cannot understand your request\r\n");
}

void
cannot_do(FILE *fp)
{
	header(fp, 501, "Not Implemented", "text/plain");
	fprintf(fp, "\r\n");

	fprintf(fp, "That command is not yet implemented\r\n");
}

void
do_404(char *item, FILE *fp)
{
	header(fp, 404, "Not Found", "text/plain");
	fprintf(fp, "\r\n");

	fprintf(fp, " The item you requested: %s is not found\r\n", 
			item);
}


void do_403(char* item, FILE* fp)
{
        header(fp, 403, "Forbidden", "text/plain");
	fprintf(fp, "\r\n");

	fprintf(fp, "No permission to access %s. \r\n", item);
}
/* ------------------------------------------------------ *
   the directory listing section
   isadir() uses stat, not_exist() uses stat
   do_ls runs ls. It should not
   ------------------------------------------------------ */

int
isadir(char *f)
{
	struct stat info;
	return ( stat(f, &info) != -1 && S_ISDIR(info.st_mode) );
}

int
not_exist(char *f)
{
	struct stat info;

	return( stat(f,&info) == -1 && errno == ENOENT );
}

int has_permission(char* f)
{
        struct stat info;
	if ( stat(f, &info) != -1 && (S_IXUSR & info.st_mode))
	     return 1;

	return 0;
}
/*
 * lists the directory named by 'dir' 
 * sends the listing to the stream as html at fp
 * if index.html exists calls do_cat
 * if index.cgi exists calls do_exec
 */
void
do_ls(char *dir, FILE *fp)
{
        int	fd;	/* file descriptor of stream */
	int     findhtml = -1;
	int     findcgi  = -1;
	//char    *dirlist = NULL;
	FLEXSTR s;

	fd = fileno(fp);
	dup2(fd,2);

	if (get_dirlist_html(dir ,&findhtml, &findcgi, &s) == -1){
	    printf("directory cannot be opened/n");
	    do_403(dir, fp);
	    return;
	}

	if (findhtml == 1){      
	    process_index_file(dir, INDEX_HTML,fp);
	}
	else if (findcgi == 1){  
	    process_index_file(dir, INDEX_CGI, fp);
	}
	else{
	    //dirlist = fs_getstr(&s);	   
	    header(fp, 200, "OK", "text/html");
	    fprintf(fp,"\r\n");
	    fflush(fp);
	    if (is_headcmd == 1) 
	        return;
	    fprintf(fp,"%s\r\n",fs_getstr(&s));
	    fs_free(&s);
	    //free(dirlist);

	    //fd = fileno(fp);
	    //dup2(fd,1);
	    //dup2(fd,2);
	    //execlp("/bin/ls","ls","-l",dir,NULL);
	    //perror(dir);	    
	}
}

/* ------------------------------------------------------ *
   the cgi stuff.  function to check extension and
   one to run the program.
   ------------------------------------------------------ */

char *
file_type(char *f)
/* returns 'extension' of file */
{
	char	*cp;
	if ( (cp = strrchr(f, '.' )) != NULL )
		return cp+1;
	return "";
}

int
ends_in_cgi(char *f)
{
	return ( strcmp( file_type(f), "cgi" ) == 0 );
}

void
do_exec( char *prog, FILE *fp)
{
	int	 fd = fileno(fp);

	header(fp, 200, "OK", NULL);
	fflush(fp);

	if (is_headcmd == 1){
	    do_head_exec(prog,fp);	   
	}
	else{
	    dup2(fd, 1);
	    dup2(fd, 2);
	    execl(prog,prog,NULL);
	    perror(prog);
	}
}
/* ------------------------------------------------------ *
   do_cat(filename,fp)
   sends back contents after a header if cmd is "GET"
   else just send the header if cmd is "HEAD"
   ------------------------------------------------------ */

void
do_cat(char *f, FILE *fpsock)
{
	char	*extension = file_type(f);
	char	*content   = find_typein_table(extension); 
	FILE	*fpfile;
	int	c;

	/*if ( strcmp(extension,"html") == 0 )
		content = "text/html";
	else if ( strcmp(extension, "gif") == 0 )
		content = "image/gif";
	else if ( strcmp(extension, "jpg") == 0 )
		content = "image/jpeg";
	else if ( strcmp(extension, "jpeg") == 0 )
	        content = "image/jpeg";
	*/

	fpfile = fopen( f , "r");
	if (fpfile != NULL )
	{
		header(fpsock, 200, "OK", content );
		fprintf(fpsock, "\r\n");

		while ( is_headcmd == 0 &&
		        (c = getc(fpfile)) != EOF)
			putc(c, fpsock);

		fclose(fpfile);
	}
	else{
	        do_403(f, fpsock);
	}
}

char *
full_hostname()
/*
 * returns full `official' hostname for current machine
 * NOTE: this returns a ptr to a static buffer that is
 *       overwritten with each call. ( you know what to do.)
 */
{
	struct	hostent		*hp;
	char	hname[MAXHOSTNAMELEN];
	static  char fullname[MAXHOSTNAMELEN];

	if ( gethostname(hname,MAXHOSTNAMELEN) == -1 )	/* get rel name	*/
	{
		perror("gethostname");
		exit(1);
	}
	hp = gethostbyname( hname );		/* get info about host	*/
	if ( hp == NULL )			/*   or die		*/
		return NULL;
	strcpy( fullname, hp->h_name );		/* store foo.bar.com	*/
	return fullname;			/* and return it	*/
}


void fatal(char *fmt, char *str)
{
        fprintf(stderr, fmt, str);
	exit(1);
}


/*
  do_head_exec - 
                 does pipe ,fork and execl
		 child process 
		 - dups one end of pipe to stdout
		 - dups the passed fp to stdin
		 - close reading end if pipe
		 - calls execl
		 - writes to the pipe
		 
		 parent process
		 - dups one end of pipe to stdin
		 - close other end of pipe
		 - reads from the end of pipe which is now stdin
		 
*/
void do_head_exec(char* prog, FILE* fp)
{
	int   pid, pipefd[2];
	char* line = NULL;
	int   fd = fileno(fp);

	if (pipe(pipefd) == -1)
	    oops("pipe failed", 1);

	if ((pid = fork())== -1)
	    oops("cannot fork", 2);

	if (pid == 0){
	    dup2(pipefd[1],1);
	    dup2(fd,0);
	    close(pipefd[1]);
	    close(pipefd[0]);
	    close(fd);
	    execl(prog,prog, NULL);
	}
	else{
	    dup2(pipefd[0],0);
	    close(pipefd[0]);
	    close(pipefd[1]);
	    //wait(NULL);	  
	    while ((line= read_response_line(stdin)) != NULL){
	         fprintf(fp,"%s\r\n",line);
		 free(line);
	    }
	    fprintf(fp,"\r\n");	  
	}
        /*
	FILE* ff;
	ff = popen(prog ,"r"); 
	if (ff == NULL)
	     oops("popen error", 1);

	 while ((line= read_response_line(ff)) != NULL){
	         fprintf(fp,"%s\r\n",line);
		 free(line);
	 }
	 fprintf(fp,"\r\n");*/	    
}


/*
  read_response_line - 
                     reads the stream from stdin line by line
		     and adds it to FLEXSTR string
		     returns the string 
		     returns NULL if a blank string is found in stream
		     which means header info is read.
*/
char* read_response_line(FILE* fp)
{
        int  c;
	int  len = 0;
	FLEXSTR s;

	fs_init(&s,0);
	while ((c = fgetc(fp)) != EOF)
	{
	       if (c == '\n')
		   break;
	       fs_addch(&s, c);
	       len++;
	}
	
	if (len == 0)
	    return NULL;

	return fs_getstr(&s);
}

/*
  set_cgi_arguments -
                     splits the string passed into one before '?'
		     and other after '?'.
		     calls the setenv() with the value after '?' for QUERY STRING varaible
*/
void  set_cgi_arguments(char* str)
{
        char* ptr = NULL;

	setenv(REQ_METHOD, "GET", 1);
	ptr = strchr(str, '?');
        if (ptr == NULL){
	  setenv(QUERY_STR,"",1);
	}
        else{
	   setenv(QUERY_STR, ptr+1, 1);
	   *ptr ='\0';
	}
}


/* 
   get_dirlist_html -
                      @dir - directory to be processed
		      @fhtml - return value if an index.html exists
		      @fcgi  - return value if an index.cgi exists
		      returns full html content as a string.
		      
		      constructs html file by adding to FLEXSTR string
		      calls opendir and readdir, performs fnmatch of file name
		      for index.html and index.cgi . if index.html exists breaks 
		      away from loop to call do_cat on html file
		      or else adds the file info to running FLEXSTR string.
		      
*/
int get_dirlist_html(char* dir, int* fhtml, int* fcgi, FLEXSTR* s)
{        
	 DIR            *dir_ptr;
	 struct dirent  *direntp;	 
	 char*          path = NULL;
	 int            ret = -1;

     	 fs_init(s, 0);
	 if ((dir_ptr = opendir( dir )) == NULL){
	      fprintf(stderr,"ws: `%s': %s\n", dir, strerror(errno));
	      printf("open dir failed. \n");
	 }
	 else {
	     if ((*fhtml = has_indexfile(dir,INDEX_HTML)) == 1)
		  ret = 0;
	     else if ((*fcgi = has_indexfile(dir, INDEX_CGI)) == -1)
	          ret = 0;
	     else {
	          fs_addstr(s,"<html><body><table>");
		  while ((direntp = readdir( dir_ptr)) != NULL) {
		        if (is_parent_cur_directory(direntp->d_name))
			  continue;  	       		     
			path = malloc(strlen(dir) + strlen(direntp->d_name) + 2);
			if (path == NULL)
			  oops("memory error",1);
		 
			sprintf (path ,"%s/%s", dir, direntp->d_name);	
			add_htmltable_rows(s, path, direntp->d_name);
			free(path);
		  }
		  fs_addstr(s,"</table></body></html>");
		  ret = 0;
	     }
	 }	 
	 return ret;
}


/*
  add_htmltable_rows
                     - @FLEXSTR s - holds the running html file contents
		                    gets modified as a result of adding a new table row
				    that contains the @file information from lstat.
				    uses a preformatted html string and adds the file info.
				    
*/
void add_htmltable_rows(FLEXSTR* s, char* path , char* file)
{
         char*           str = NULL;
	 char            mdate[MAXDATELEN] = "";
	 char            fsize[MAXFILESIZE] = "";
	 /* html format string */
	 char*           fmt = "<tr><td valign=\"top\"></td><td><a href=%s>%s</a></td>"\
                                "<td align=\"right\">%s   </td>"\
                                "<td align=\"right\">   %s</td>"\
	                        "<td>&nbsp;</td></tr>";
	 struct stat     info;
         	 
	 get_file_info(&info, path, mdate, fsize);
	 str = malloc(strlen(fmt) + strlen(path) +strlen(file) +
	              strlen(mdate) + strlen(fsize) + 1);

	 if (str == NULL)
	     oops("memory error", 1);

	 sprintf(str, fmt, path, file, mdate, fsize);
	 fs_addstr(s, str);
	 free(str);
}



/* 
   get_file_info -
                   calls lstat on @path to get the file info 
		   like last modified time and file size
		   converts timespec to string using strftime
		   passes back the date string and file size string

*/
void get_file_info(struct stat* info, char* path, char* mdate, char* filesz)
{
         struct tm* tt;
         if (lstat(path, info) == -1){
	     fprintf(stderr,"ws: `%s': %s\n", path, strerror(errno));
	     return;
	 }
	 tt = localtime(&info->st_mtim.tv_sec);
	 strftime(mdate, MAXDATELEN, DATE_FMT,tt);

	 if (S_ISDIR(info->st_mode))
	     strcpy(filesz,"-");
	 else
	     sprintf(filesz,"%d",(int)info->st_size);	       

}

/*
  process_index_file -
                      @dir - directory that is passed in GET cmd
                      @indexfile- can be index.html or index.cgi
		      
                      call do_cat() or do_exec() based on index file
                      
 */
void process_index_file(char* dir, char* indexfile, FILE* fp)
{
        char *file = malloc(strlen(dir) + strlen(indexfile) + 2);
	if (file == NULL){
	    oops("memory error", 1);
	}
	
	 sprintf(file,"%s/%s", dir, indexfile);

	 if (strcasecmp(indexfile,INDEX_HTML) == 0)
	     do_cat(file,fp);
	 else if (strcasecmp(indexfile, INDEX_CGI) == 0)
	     do_exec(file,fp);

	 free(file);
        
}


int is_parent_cur_directory(char* fname)
{
         if (fname[0] == '.' && strlen( fname) == 1)
	     return 1;

	 if (fname[0] == '.' && fname[1] == '.' && strlen( fname) == 2)
	     return 1;

	 return 0;
}


/* 
   handle_child_exit - handler for SIGCHLD signal
*/
void handle_child_exit(int signum)
{
         while (waitpid(-1,NULL,WNOHANG) > 0)
	      ;
}

/*

  block_sigchld
                - block SIGCHLD signal before 'accept' system call
		  and unblock signal after 'accept' system call
 */

void block_sigchld(int block)
{
         sigset_t sigs;
	 sigemptyset(&sigs);
	 sigaddset(&sigs, SIGCHLD);
	 
	 if (block == 1)
	     sigprocmask(SIG_BLOCK, &sigs, NULL);

	 if (block == 0)
	     sigprocmask(SIG_UNBLOCK, &sigs, NULL);
  
}

int has_indexfile(char* dir, char* indexfile)
{
         struct stat info;
	 int ret = 0;
         char* file = malloc(strlen(dir) + strlen(indexfile) +2);
	 if (file == NULL)
	     oops("memory error", 1);
  
	 sprintf(file,"%s/%s",dir, indexfile);
	 if (stat(file,&info) != -1 && errno != ENOENT)
	     ret = 1;;

	 free(file);
	 return ret;
}
