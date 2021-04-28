/*
 * struct for paddle
 *
*/

struct pppaddle{
  int   pad_top,
        pad_bot,
        pad_col;
  char  pad_char; 

};

void paddle_init();
void paddle_up();
void paddle_down();
int  paddle_contact(int y, int x);
