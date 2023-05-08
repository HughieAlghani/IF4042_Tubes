#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libproc/proc_inter.c,v 11.0 1992/07/21 18:22:21 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table giving interactive top level procedures
 *1 local.inter.top
 *2 * (unused1, unused2, inst)
 *3    char *unused1;
 *3    char *unused2;
 *3    int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 As top-level procedures, these procedures are responsible for setting
 *7 trace conditions, and for determining other execution time constraints,
 *7 such as when execution should stop (eg, if global_end is exceeded).
 *7 These procedures are the main user interactive procedures, with all
 *7 sorts of user-level commands available.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "inter_web".
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "proc.h"

extern int init_inter_web(), inter_web(), close_inter_web();
PROC_TAB lproc_inter[] = {
    "inter_web",    init_inter_web,  inter_web,    close_inter_web,
};
int num_lproc_inter =
    sizeof (lproc_inter) / sizeof (lproc_inter[0]);


