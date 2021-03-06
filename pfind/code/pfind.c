/* pfind.c
 *           search for files in a directory
 *           if a type is specified - show only those file types
 *           if a file pattern is specified - show only those file names.
 */
#include    <stdio.h>
#include    <sys/types.h>
#include    <dirent.h>
#include    <sys/stat.h>
#include    <string.h>
#include    <time.h>
#include    <errno.h>
#include     <stdlib.h>
#include     "fnmatch.h"

#define   OPTIONS_INDEX    2

static void  search_dir( char* , char* , char );
static void  find_options( int , char**, char** , char* );
static int   do_match(struct stat*, char* , char* , char* , char );
static int   match_type( struct stat  *, char );
static int   match_pattern( char *, char *);
static void  do_search( char* , char* , char* , char );
static int   is_parent_cur_directory( char* fname);
static int   check_filetype( char );

int main(int ac, char *av[])
{
    char      *dir = NULL;
    char      *file_pattern = NULL;
    char      file_type = 0;

    if (ac > 1 && av[1] != NULL) {
        dir = av[1];
    }
    else {
        fprintf(stderr,"pfind: missing directory to process\n");
        exit(1);
    }

    find_options(ac, av, &file_pattern, &file_type);

    search_dir(dir, file_pattern, file_type);

    return 0;
}

/*find_opions - process the command arguments
                and gets filename and type
*/  
void find_options(int nargs, char *args[],  char** filename, char* typearg)
{
    int cur_index = OPTIONS_INDEX;
    int file_argno = -1;                /* file pattern argument index*/
    int type_argno = -1;                /* type argument index*/
    
    while (cur_index < nargs) {         /*loop to check what options are passed */
        if (strcmp(args[cur_index], "-name") == 0) {
            if (file_argno == -1)       /*get file name index*/
                file_argno = ++cur_index;
            else {
                fprintf(stderr,"pfind:multiple `-name' options\n");
                exit(1);
            }
        }
        else if (strcmp(args[cur_index], "-type") == 0) {
            if (type_argno == -1)       /* get file type index*/
                type_argno = ++cur_index;
            else {
                fprintf(stderr,"pfind: multiple `-type' options\n");
            }
        }
        else {                          /* incorrect option specified*/
            fprintf(stderr, "pfind: unknown predicate `%s'\n", args[cur_index]);
            exit(1);
        }
        cur_index++;
    }

    if (file_argno != -1 ) {            /* get file pattern if -name is specified*/
        if (nargs > file_argno ) {
            *filename = args[file_argno];
        }
        else {                          /*insufficient argument to get file pattern*/
            fprintf(stderr,"pfind: missing argument to `-name'\n" );
            exit(1);
        }
    }

    if (type_argno != -1 ) {            /* get file type if -type specified*/
        if (nargs > type_argno && strlen (args[type_argno]) == 1) {
            *typearg =  args[type_argno][0];
        }
        else {
            fprintf(stderr, "pfind: missing argument to `-type'\n");
            exit(1);
        }

        if (!check_filetype(*typearg)) {
            fprintf(stderr, "pfind: incorrect argument to `-type'\n");
            exit(1);
        }
    }
}

/*
 searchdir - checks whether the passed in directory is valid
             sets the stage for recursion
*/
void search_dir(char* dirname, char* findme, char type)
{
    if (dirname == NULL ||  dirname[0] == '\0')
        return;

    do_search(dirname, dirname, findme, type);
}

/*
  do_search - opens directories (use opendir()), get DIR pointer,
              reads each directory entry (readdir())
              get struct dirent pointer
              appends the entry to the current directory path
              does lstat on entry and if succeeds
              recurses on each directory entry
              closes directories when recursion back tracks.
*/
void do_search(char* dir, char* basefile, char* findme, char type)
{
    DIR             *dir_ptr;           /* the directory */
    struct dirent   *direntp;           /* each entry	 */
    char            *path;

    struct stat     info;

    if (do_match(&info, dir, basefile, findme, type) == -1)
        return;

    if (!S_ISDIR(info.st_mode) )
        return;

    if ( ( dir_ptr = opendir( dir ) ) == NULL )
        fprintf(stderr,"pfind: `%s': %s\n", dir, strerror(errno));
    else {
        while ( (direntp = readdir( dir_ptr ) ) != NULL ) {
            if (is_parent_cur_directory( direntp->d_name))
                continue;

            path = malloc(strlen(dir) + strlen(direntp->d_name) + 2);
            if (path == NULL) {
                fprintf(stderr,"pfind: Not enough memory\n");
                exit(1);
            }
            sprintf (path ,"%s/%s", dir, direntp->d_name);

            do_search( path ,direntp->d_name, findme, type);

            free(path);

        }

        closedir(dir_ptr);
    }
}

/*
  do_match - calls lstat and if succeeds tries to
             match file type and file pattern
             if both match then prints the path represented by filepath
         @param info  - holds the struct stat data
         @param filepath - holds full path to directory
         @param name   - file name to be used to match the pattern
         @param findme - pattern passed by user
         @param type - file type passed by user
         @return  -1 if lstat fails or else returns 0.
*/
int do_match(struct stat* info, char *filepath ,char* name, char* findme, char type)
{
    if (lstat(filepath, info) == -1 ) { /* cannot stat */
        fprintf(stderr,"pfind: `%s': %s\n", filepath, strerror(errno));
        return -1;
    }
    else {                              /* else show info */
        if (match_type( info , type ) == 0 &&
            match_pattern( name, findme ) == 0)
            printf("%s\n", filepath);

    }

    return 0;
}

/*
match_type - uses defines in sys/stat to match the
             type passed in by user with current file type
         @param info - struct stat for getting the mode
         @param type - type to match with
             if succeeds returns 0 else returns  -1
             note: if no type is specified by user function
                   just returns 0;

 */
int match_type(struct stat* info, char type)
{
    if      (type == 'f' && S_ISREG( info->st_mode) )
        return 0;
    else if (type == 'd' && S_ISDIR( info->st_mode) )
        return 0;
    else if (type == 'b' && S_ISBLK( info->st_mode) )
        return 0;
    else if (type == 'c' && S_ISCHR( info->st_mode) )
        return 0;
    else if (type == 'p' && S_ISFIFO( info->st_mode) )
        return 0;
    else if (type == 'l' && S_ISLNK( info->st_mode) )
        return 0;
    else if (type == 's' && S_ISSOCK( info->st_mode) )
        return 0;
    else if (type == 0)
        return 0;
    else
        return -1;

}

/*
  match_pattern - uses fnmatch() function to
                  match the file pattern passed in by user
                  and current file entry in the directory
          @param file - filename to match to pattern
          @param pattern - pattern specified by user
                  returns 0 if succeeds else returns -1
 */
int match_pattern(char *file,  char* pattern)
{
    if (pattern == NULL || pattern[0] == '\0')
        return 0;

    if (fnmatch( pattern, file, FNM_PATHNAME) == 0)
        return 0;

    return -1;

}

/*
is_parent_cur_directory - check if directory is '.' and '..'
                         if succeeds returns 1 else 0
                         check is done to avoid recursion to go to infinite loop

*/
int is_parent_cur_directory(char* fname)
{
    if (fname[0] == '.' && strlen( fname) == 1)
        return 1;

    if (fname[0] == '.' && fname[1] == '.' && strlen( fname) == 2)
        return 1;

    return 0;

}

/*
check_filetype - check the type passed is one of {f|b|c|d|l|s|p}
                 if it is one of the above return 1 else return 0
*/
int check_filetype(char type)
{

    if (type == 'f' ||type == 'c' || type == 'd' ||
        type == 'p' || type == 'b' || type == 's' || type == 'l')
        return 1;

    return 0;

}
