

struct flaginfo {
  const char  *fl_name;  /* holds name of flag name*/
  tcflag_t    fl_value; /* holds the actual represntation in termios*/
};


extern struct flaginfo input_flags[];
extern struct flaginfo output_flags[];
extern struct flaginfo control_flags[];
extern struct flaginfo local_flags[];
extern struct flaginfo c_ccflags[];
