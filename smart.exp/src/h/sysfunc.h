/*        $Header: /home/smart/release/src/h/sysfunc.h,v 11.0 1992/07/21 18:18:55 chrisb Exp $ */
/* Declarations of major functions within standard C libraries */
/* Once all of the major systems get their act together (and I follow
   suit!), this file should just include system header files from 
   /usr/include.  Until then... */

#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#ifdef MMAP
int munmap();
#endif

