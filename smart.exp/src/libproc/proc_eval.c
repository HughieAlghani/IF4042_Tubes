#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libproc/proc_eval.c,v 11.0 1992/07/21 18:23:05 chrisb Exp $";
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
 *0 Hierarchy table giving evaluation routines
 *1 evaluate
 *2 * (unused, arg2, inst)
 *3   char *arg1;
 *3   SM_EVAL *arg2;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Return UNDEF if error.

 *8 Current possibilities are "tr_eval", "rr_eval"
***********************************************************************/
extern int init_tr_eval(), tr_eval(), close_tr_eval();
extern int init_rr_eval(), rr_eval(), close_rr_eval();
extern int init_tr_oeval(), tr_oeval(), close_tr_oeval();
extern int init_rr_oeval(), rr_oeval(), close_rr_oeval();
PROC_TAB proc_eval[] = {
    "tr_eval",		init_tr_eval,	tr_eval,	close_tr_eval,
    "rr_eval",		init_rr_eval,	rr_eval,	close_rr_eval,
    "tr_oeval",		init_tr_oeval,	tr_oeval,	close_tr_oeval,
    "rr_oeval",		init_rr_oeval,	rr_oeval,	close_rr_oeval,
    };
int num_proc_eval = sizeof (proc_eval) / sizeof (proc_eval[0]);
