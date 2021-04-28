#include	<stdio.h>
#include	<stdlib.h>
#include        <string.h>
#include	<strings.h>
#include        "table.h"



static struct ftype* table = NULL;
static int cur_size = 0;
static int tot_size = 10;

static void reinit_table();

char* find_typein_table(char* type)
{
        int i = -1;
	int found = 0;
	for (i = 0; i < cur_size; i++){
	    if (strcasecmp(type, table[i].ext_type) == 0){
	      found = 1;
	      break;
	    }
	}
  
	if (found == 0){
	  return table[0].content_type;
	}
	return table[i].content_type;
}


void print_table()
{
        int  i;
	if (table == NULL)
	  return;

	for (i = 0; i <cur_size ; i++)
	  printf("type : %s; description: %s\r\n",
		 table[i].ext_type, table[i].content_type);
  
}

/*void reinit_table()
{
        if (table == NULL){
	  table = (struct ftype**)calloc(tot_size , sizeof(struct ftype*));
	  if (table == NULL){
	     fprintf(stderr,"memory error");
	     exit(1);
	  }	    
	}

	if (cur_size == tot_size){
	  tot_size *= 2;
	  table = (struct ftype**)realloc(table,tot_size);
	  if (table == NULL){
	    fprintf(stderr,"memory error");
	    exit(1);
	  }
	}
}
void add_type_table(char* value, char* descr)
{
        struct ftype* item = NULL;

	reinit_table();
	item = malloc(sizeof (struct ftype));  
	if (item == NULL){
	  fprintf(stderr,"memory error");
	  exit(1);
	}
	item->ext_type = strdup(value);
	item->content_type = strdup(descr); 
	table[cur_size] = item;
	cur_size++;
}*/

void release_table()
{
        int  i;
        for (i = 0; i < cur_size; i++){
	     free(table[i].ext_type);
	     free(table[i].content_type);	    	  
	}
	free(table);
	table = NULL;	  
}

void reinit_table()
{
        if (table == NULL){
	   table = malloc(tot_size*sizeof(struct ftype));
	  if (table == NULL){
	     fprintf(stderr,"memory error");
	     exit(1);
	  }	    
	}

	if (cur_size == tot_size){
	    tot_size *= 2;
	    table = realloc(table, tot_size*sizeof(struct ftype));
	    if (table == NULL){
	      fprintf(stderr,"memory error");
	      exit(1);
	  }
	}
}

void add_type_table(char* value, char* descr)
{
	reinit_table();
	table[cur_size].ext_type = strdup(value);
	table[cur_size].content_type = strdup(descr); 
	cur_size++;
}
