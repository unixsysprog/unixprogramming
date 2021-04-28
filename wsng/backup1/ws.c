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
#include        "fnmatch.h"
#include        "flexstr.h"
#include        "webtime.h"
#include	"socklib.h"
#include        "table.h"

/*
 * ws.c - a web server
 *
 *    usage: ws [ -c configfilenmame ]
 * features: supports the GET command only
 *           runs in the current directory
 *           forks a new child to handle each request
 *           needs many additional features 
 *
 *  compile: cc ws.c socklib.c -o ws
 *  history: 2012-04-23 removed extern declaration for fdopen (it's in stdio.h)
 *  history: 2012-04-21 more minor cleanups, expanded some fcn comments
 *  history: 2010-04-24 cleaned code, merged some of MK's ideas
 *  history: 2008-05-01 removed extra fclose that was causing double free
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


static int is_headcmd = 0;

/*
 * prototypes
 */

int	startup(int, char *a[], char [], int *);
void	read_til_crnl(FILE *);
void	process_rq( char *, FILE *);
void	bad_request(FILE *);
void	cannot_do(FILE *fp);
void	do_404(char *item, FILE *fp);
void	do_cat(char *f, FILE *fpsock);
void	do_exec( char *prog, FILE *fp, int , char** );
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
static  void  has_index_file(char* , int* , int* );
static  void  process_index_file(char* , char* , FILE* );
static  char* read_response_line(FILE* );
static  void  do_head_exec(char* ,FILE* );
static  void  set_cgi_arguments(char* , FLEXLIST* );
static  char* get_dirlist_html(char* );
static  void  get_file_info(struct stat* , char* , char* , char* );

//static  char**  set_cgi_arguments(char* , int* );

void    handle_child_exit(int );

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
		fd    = accept( sock, NULL, NULL );	/* take a call	*/
		if ( fd == -1)
			perror("accept");
		else
			handle_call(fd);		/* handle call	*/
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
		if (strcasecmp(param,"type") == 0)
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
	char      **envar;
	FLEXLIST  list;
	int       nargs = 0;

	if (sscanf(rq, "%s%s", cmd, arg) != 2 ){
		bad_request(fp);
		return;
	}

	if (strcmp(cmd,"HEAD") == 0)
	    is_headcmd = 1;

	item = modify_argument(arg, MAX_RQ_LEN);
	
	fl_init(&list,0);
	set_cgi_arguments(item, &list);
	nargs = fl_getcount(&list);
	if (nargs > 0){
	    envar = fl_getlist(&list);
	}

	if (strcmp(cmd,"GET") != 0 && is_headcmd != 1 )
		cannot_do(fp);
	else if (not_exist(item ) )
		do_404(item, fp );
	else if (isadir( item ) )
		do_ls(item, fp );
	else if (ends_in_cgi(item ) )
	        do_exec(item, fp, nargs, envar);
	else
		do_cat(item, fp );

        is_headcmd =  0;
	fl_free(&list);
}

/*
 * modify_argument
 *  purpose: many roles
 *		security - remove all ".." components in paths
 *		cleaning - if arg is "/" convert to "."
 *  returns: pointer to modified string
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

	fprintf(fp, "The item you requested: %s\r\nis not found\r\n", 
			item);
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

/*
 * lists the directory named by 'dir' 
 * sends the listing to the stream at fp
 */
void
do_ls(char *dir, FILE *fp)
{
	int	fd;	/* file descriptor of stream */
	int     findhtml = -1;
	int     findcgi  = -1;
	char    *dirlist = NULL;

	has_index_file(dir,&findhtml,&findcgi);

	if (findhtml == 1){       /*found index.html,call do_cat()*/
	    process_index_file(dir, INDEX_HTML,fp);
	}
	else if (findcgi == 1){   /*found index.cgi,call do_exec()*/
	    process_index_file(dir, INDEX_CGI, fp);
	}
	else{
	    //header(fp, 200, "OK", "text/plain");
	    header(fp, 200, "OK", "text/html");
	    fprintf(fp,"\r\n");
	    fflush(fp);

	    if (is_headcmd == 1) /*if it is a head cmd dont call exec */
	        return;

	    dirlist = get_dirlist_html(dir);
	    fprintf(fp,"%s\r\n",dirlist);
	    free(dirlist);
	    /*fd = fileno(fp);
	    dup2(fd,1);
	    dup2(fd,2);
	    execlp("/bin/ls","ls","-l",dir,NULL);
	    perror(dir);*/	    
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
do_exec( char *prog, FILE *fp, int nargs, char** args)
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
	    if (nargs > 0){	      
	        execle(prog,prog,NULL,args);
	    }
	    else{
	        execl(prog,prog,NULL);
	    }
	    perror(prog);
	}
}
/* ------------------------------------------------------ *
   do_cat(filename,fp)
   sends back contents after a header
   ------------------------------------------------------ */

void
do_cat(char *f, FILE *fpsock)
{
	char	*extension = file_type(f);
	char	*content   = find_typein_table(extension); //"text/plain";
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
	if ( fpfile != NULL )
	{
		header(fpsock, 200, "OK", content );
		fprintf(fpsock, "\r\n");

		while ( is_headcmd == 0 &&
		        (c = getc(fpfile)) != EOF)
			putc(c, fpsock);

		fclose(fpfile);
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



void handle_child_exit(int signum)
{
        while (waitpid(-1,NULL,WNOHANG) > 0)
	      ;
}


/* 
   has_index_file - 
                    @fhtml - if index.html exists,pointer is set to 1
		    @fcgi  - if index.cgi exists, pointer is set to 1
                    calls opendir and readdir to find the
                    files in directory.even if index.cgi exists 
		    will continue to search if index.html exists.
		    but if index.html exists, loop will break
		    use fnmatch to compare the file names.
		    

*/
void has_index_file(char* dir, int* fhtml, int* fcgi)
{

        DIR            *dir_ptr;
	struct dirent  *direntp;

	if ((dir_ptr = opendir( dir )) == NULL)
	     fprintf(stderr,"ws: `%s': %s\n", dir, strerror(errno));
	else {
	    while ((direntp = readdir( dir_ptr)) != NULL) {
                if (is_parent_cur_directory(direntp->d_name))
		    continue;
		if (fnmatch(direntp->d_name,INDEX_HTML,0)== 0){
		    *fhtml = 1;
		    break;
		}
		if (fnmatch(direntp->d_name,INDEX_CGI,0) == 0){
		    *fcgi = 1;
		}     
	  }      
        }
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
	     do_exec(file,fp, 0, NULL);

	 free(file);
        
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
	    wait(NULL);	  
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

char* read_response_line(FILE* fp)
{
        int  c;
	int  len = 0;
	FLEXSTR s;

	fs_init(&s,0);
	while ((c = fgetc(fp)) != EOF)
	{
	       if (c == '\r' || c == '\n')
		   break;
	       fs_addch(&s, c);
	       len++;
	}
	
	if (len == 0)
	    return NULL;

	return fs_getstr(&s);
}

void  set_cgi_arguments(char* str, FLEXLIST* list)
{
        char* ptr = NULL;
	char* arg1 = NULL;
	char* arg2 = NULL;

	ptr = strrchr(str, '?');

        if (ptr == NULL)
	   return;
       
	arg1 =  malloc(strlen(REQ_METHOD) + strlen("GET") + 2);
	if (arg1 == NULL)
	    oops("memory error",1);

	sprintf(arg1,"%s=%s",REQ_METHOD,"GET");
	fl_append(list,arg1);

	arg2 = malloc(strlen(QUERY_STR) + strlen(ptr+1) +2);
	if (arg2 == NULL)
	    oops("memory error" ,1);

	sprintf(arg2,"%s=%s",QUERY_STR,ptr+1);
	fl_append(list,arg2 );

	fl_append(list, NULL);

	*ptr = '\0';

}

char* get_dirlist_html(char* dir)
{        
         FLEXSTR        s;
	 DIR            *dir_ptr;
	 struct dirent  *direntp;
	 char*           fmt = "<tr><td valign=\"top\"></td><td><a href=%s>%s</a></td><td align=\"right\">%s   </td><td align=\"right\">   %s</td><td>&nbsp;</td></tr>";
	 char*           str = NULL;
	 char*           path = NULL;
	 char            mdate[MAXDATELEN] = "";
	 char            fsize[MAXFILESIZE] = "";
	 struct stat     info;

	 fs_init(&s, 0);
	 fs_addstr(&s,"<html><body><table>");
	 
	 if ((dir_ptr = opendir( dir )) == NULL)
	     fprintf(stderr,"ws: `%s': %s\n", dir, strerror(errno));
	 else {
	    while ((direntp = readdir( dir_ptr)) != NULL) {
                if (is_parent_cur_directory(direntp->d_name))
		    continue;		
		 path = malloc(strlen(dir) + strlen(direntp->d_name) + 2);
		 if (path == NULL)
		   oops("memory error",1);
		 
		 sprintf (path ,"%s/%s", dir, direntp->d_name);		 
		 get_file_info(&info, path, mdate, fsize);
		 str = malloc(strlen(fmt) + strlen(path) +strlen(direntp->d_name) +
		              strlen(mdate) + strlen(fsize) + 1);
		 sprintf(str, fmt, path, direntp->d_name, mdate, fsize);
		 if (str == NULL)
		     oops("memory error", 1);

		 fs_addstr(&s, str);
		 free(str);
		 free(path);
	    }
	 }
	 fs_addstr(&s,"</table></body></html>");
	 return fs_getstr(&s);
}

void get_file_info(struct stat* info, char* path, char* mdate, char* filesz)
{
         struct tm* tt;
         if (lstat(path, info) == -1){
	     oops("stat failed",1);
	 }
	 tt = localtime(&info->st_mtim.tv_sec);
	 strftime(mdate, MAXDATELEN, DATE_FMT,tt);

	 if (S_ISDIR(info->st_mode))
	     strcpy(filesz,"-");
	 else
	   sprintf(filesz,"%d",(int)info->st_size);	       

}





/*char**  set_cgi_arguments(char* str, int* nargs)
{
        char* ptr = NULL;	
	char** args;

	ptr = strrchr(str, '?');

        if (ptr == NULL)
	   return NULL;

	args = malloc(3*sizeof(char*));
	*nargs = 3;

        args[0] = malloc(strlen(REQ_METHOD) + strlen("GET") + 2);
	sprintf(args[0],"%s=%s",REQ_METHOD,"GET");

	args[1] = malloc(strlen(QUERY_STR) + strlen(ptr+1) +2);
	sprintf(args[1],"%s=%s",QUERY_STR,ptr+1);
	
	args[2] = NULL;

	*ptr = '\0';
	printf("program is: %s\n",str);

	return args;

}*/
