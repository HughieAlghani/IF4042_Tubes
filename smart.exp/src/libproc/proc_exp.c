#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libproc/proc_exp.c,v 11.0 1992/07/21 18:23:05 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "proc.h"

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table giving generic procedure names
 *1 exp
 *2 * (arg1, arg2, inst)
 *3   char *arg1;
 *3   char *arg2;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedure names provide an easy mechanism for temporarily
 *7 linking a user defined replacement procedure into smart.  The idea is
 *7 for a student (or experimenter) to write a hierarchy procedure (ie
 *7 triplet) with procedure names init_exp1(), exp1(), and close_exp1().
 *7 The student then links that procedure along with the main procedure
 *7 smart.o and the smart libraries 
 *7 (eg "cc -o mysmart myproc.o ...src/smart/smart.o all_the_smart_libs")
 *7 Their procedure can then be referred to in their specification file as
 *7 the hierarchy procedure "exp.1", and used however is desired.  
 *7 This allows easy construction of temporary versions of smart, 
 *7 for example in a classroom environment.
 *7 Return UNDEF if error.

 *8 Current possibilities are "1", "2", "3", "4"
***********************************************************************/
extern int init_exp1(), exp1(), close_exp1();
extern int init_exp2(), exp2(), close_exp2();
extern int init_exp3(), exp3(), close_exp3();
extern int init_exp4(), exp4(), close_exp4();
PROC_TAB proc_exp[] = {
    "1",		init_exp1,	exp1,	close_exp1,
    "2",		init_exp2,	exp2,	close_exp2,
    "3",		init_exp3,	exp3,	close_exp3,
    "4",		init_exp4,	exp4,	close_exp4,
    };
int num_proc_exp = sizeof (proc_exp) / sizeof (proc_exp[0]);
