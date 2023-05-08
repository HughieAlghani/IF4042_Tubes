#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libproc/proc.c,v 11.0 1992/07/21 18:22:20 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Root of local hierarchy table giving other tables of procedures
 *1 local
 *7 Procedure table mapping a string procedure table name to table addresses.
 *8 Current possibilities are "none", 
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "proc.h"
#include "spec.h"

extern PROC_TAB lproc_inter[];
extern int num_lproc_inter;
extern TAB_PROC_TAB lproc_index[];
extern int num_lproc_index;
extern TAB_PROC_TAB lproc_convert[];
extern int num_lproc_convert;
extern TAB_PROC_TAB lproc_retrieve[];
extern int num_lproc_retrieve;
extern TAB_PROC_TAB lproc_print[];
extern int num_lproc_print;
/* extern PROC_TAB lproc_exp[]; */
/* extern int num_lproc_exp; */
/* extern TAB_PROC_TAB lproc_feedback[]; */
/* extern int num_lproc_feedback; */
/* extern PROC_TAB lproc_eval[]; */
/* extern int num_lproc_eval; */
/* #ifdef BFLY */
/* extern TAB_PROC_TAB lproc_bfly[]; */
/* extern int num_lproc_bfly; */
/* #endif */
/*  */
TAB_PROC_TAB lproc_root[] = {
     "inter",	TPT_PROC,	NULL,		lproc_inter,&num_lproc_inter,
     "index",	TPT_TAB,	lproc_index,	NULL,	&num_lproc_index,
     "convert",	TPT_TAB,	lproc_convert,   NULL,	&num_lproc_convert,
     "retrieve",TPT_TAB,	lproc_retrieve,  NULL,	&num_lproc_retrieve, 
     "print",	TPT_TAB,	lproc_print,     NULL,	&num_lproc_print,
/*     "eval",	TPT_PROC,	NULL,		lproc_eval,&num_lproc_eval, */
/*     "feedback",	TPT_TAB,	lproc_feedback,  NULL,	&num_lproc_feedback, */
/*     "exp",	TPT_PROC,	NULL,		lproc_exp,&num_lproc_exp, */
/*     "local",	TPT_TAB,	lproc_local,	NULL,	&num_lproc_local, */
/* #ifdef BFLY */
/*     "bfly",	TPT_TAB,	lproc_bfly,	NULL,	&num_lproc_bfly, */
/* #endif */
    };

int num_lproc_root = sizeof (lproc_root) / sizeof (lproc_root[0]);
