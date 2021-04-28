#include	<stdio.h>
#include        <stdlib.h>
#include	"string.h"
#include        "smsh.h"
#include        "dllist.h"

struct ifstatus* list = NULL; /* tail of linked list*/
static int nitems = 0;

/*
  add_item_list 
   - mallocs object of struct ifstatus (creates a node)
     if tail object is null make the new object tail of the list.
     sets the passed values to the members of struct
     adds the node to the end of the list 
     makes the node the new tail of the list.
*/
void add_item_list(int key , int result, int state)
{
    struct ifstatus *item =  emalloc(sizeof (struct ifstatus));

    item->key       = key; 
    item->if_state  = state;
    item->if_result = result;
    item->prev      = NULL;
    item->next      = NULL;

    if (list == NULL){
      list        = item;
      list->prev  = NULL;
      list->next  = NULL;
    }
    else{
      list->next  = item;
      item->prev  = list;
      item->next  = NULL;
      list        = item;
    }
   
    nitems++;
}


/*
  removes_item_list
           - once an "fi" is read , pops out all the nodes of that if block
           - starts from tail and removes the nodes
	   - once it reaches node with key "if" , removes it and returns
	   - or if it reaches the first node, removes it and returns.
 */
void remove_item_list()
{
    int key;
    struct ifstatus* item;
    
    if (list  == NULL)
      return;
    
    while (list != NULL){
      key        = list->key;
      item       = list;
      list       = item->prev;
      nitems--;
      if (list == NULL){
	free(list);
	list = NULL;
	break;
      }
      else{
	list->next = NULL;
	free(item);
      }
      if (key == 1)
	break;
    }
}

/*
  get_list_state- 
                returns the state of the tail
		if list is empty returns 0
*/
int get_list_state()
{
    if (list == NULL )
      return 0;

    return list->if_state;
}


/*
  get_list_result-
                 returns the result of the tail
		 if list is empty returns  -1
*/
int get_list_result()
{
    if (list == NULL )
      return -1;

    return list->if_result;
}

/*
  is_list_empty - 
                returns 1 if tail is null or number of items =0
		else returns 0
*/
int is_list_empty()
{
    if (list == NULL || nitems <= 0)
      return 1;

    return 0; 
}


/*
  print_list -
              for debug , prints out the nodes in the list.
 */
void print_list()
{
    struct ifstatus* head = list;
    if (head == NULL) return;

    while (head->prev != NULL)
      head = head->prev;
  
    while (head != NULL){
      printf("printing list- key: %d ;result: %d; state - %d\n",
    	   head->key,head->if_result,head->if_state);
      head = head->next;
    }
}


