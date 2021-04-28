
struct ifstatus{
  int key;
  int if_state;
  int if_result;
  struct ifstatus* next;
  struct ifstatus* prev;
};



void add_item_list(int k , int r, int s);
void remove_item_list();
int  get_list_state();
int  get_list_result();
int  is_list_empty();
void print_list();


