#include        <stdio.h>
#include        <termios.h>
#include        "flags.h"

/*array that holds different flags of  struct termios c_iflag*/
struct flaginfo input_flags[] = {
   { "ignbrk" , IGNBRK  },
   { "brkint" , BRKINT  },
   { "ignpar" , IGNPAR  },
   { "parmrk" , PARMRK  },
   { "inpck"  , INPCK   },
   { "istrip" , ISTRIP  },
   { "inlcr"  , INLCR   },
   { "igncr"  , IGNCR   },
   { "icrnl"  , ICRNL   },
   { "ixon"   , IXON    },
   { "ixany"  , IXANY   },    
   { "ixoff"  , IXOFF   },
   {  NULL    , 0       }};

/*array that holds different flags of  struct termios c_lflag*/
struct flaginfo local_flags[] = {
   { "isig"  ,  ISIG   },
   { "icanon",  ICANON },
   { "echo"  ,  ECHO   },
   { "echoe" ,  ECHOE  },
   { "echok" ,  ECHOK  },
   { "iexten",  IEXTEN },
   {  NULL   ,  0      } };

/*array that holds different flags of  struct termios c_oflag*/
struct flaginfo output_flags[] = {
   {  "opost" , OPOST  },
   {  "olcuc" , OLCUC  },
   {  "onlcr" , ONLCR  },
   {  "ocrnl" , OCRNL  },
   {  NULL    , 0      } };

/*array that holds different flags of  struct termios c_cflag*/
struct flaginfo control_flags[] = {
   {"hupcl"  ,  HUPCL  },
   {"parenb" ,  PARENB },
   {"cread"  ,  CREAD  },
   { NULL    ,  0      }};

/*array that holds different elements of controls of struct termios c_cc array*/
struct flaginfo c_ccflags[] = {
   {"eof"    ,  VEOF    },
   {"eol"    ,  VEOL    },
   {"erase"  ,  VERASE  },
   {"intr"   ,  VINTR   },
   {"kill"   ,  VKILL   },
   {"start"  ,  VSTART  },  
   {"stop"   ,  VSTOP   },
   {"werase" ,  VWERASE  }, 
   { NULL    ,  0        } };

