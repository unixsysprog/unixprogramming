

struct ftype{
       char* ext_type;
       char* content_type;
};


char* find_typein_table(char*  );
void  add_type_table(char* , char*  );
void  release_table();
void  print_table();
