#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libproc/proc_fdbk.c,v 11.0 1992/07/21 18:23:06 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "proc.h"

extern int init_feedback(), feedback(), close_feedback();
static PROC_TAB topproc_feedback[] = {
    "feedback", init_feedback,   feedback,    close_feedback,
    "empty",      EMPTY,         EMPTY,       EMPTY,
};
static int num_topproc_feedback =
    sizeof(topproc_feedback) / sizeof(topproc_feedback[0]);


extern int init_exp_rel_doc(), exp_rel_doc(), close_exp_rel_doc();
extern int init_exp_const(), exp_const(), close_exp_const();
static PROC_TAB proc_expand[] = {
   "exp_rel_doc", init_exp_rel_doc,   exp_rel_doc,    close_exp_rel_doc,
   "exp_const", init_exp_const,   exp_const,    close_exp_const,
    "empty",      EMPTY,         EMPTY,       EMPTY,
};
static int num_proc_expand =
    sizeof(proc_expand) / sizeof(proc_expand[0]);

extern int init_occ_inv(), occ_inv(), close_occ_inv();
extern int init_occ_ide(), occ_ide(), close_occ_ide();
static PROC_TAB proc_occ_info[] = {
   "inv",      init_occ_inv,   occ_inv,    close_occ_inv,
   "ide",      init_occ_ide,   occ_ide,    close_occ_ide,
    "empty",      EMPTY,         EMPTY,       EMPTY,
};
static int num_proc_occ_info =
    sizeof(proc_occ_info) / sizeof(proc_occ_info[0]);

extern int init_wt_fdbk(), wt_fdbk(), close_wt_fdbk();
extern int init_wt_ide(), wt_ide(), close_wt_ide();
static PROC_TAB proc_weight[] = {
    "fdbk",       init_wt_fdbk,  wt_fdbk,   close_wt_fdbk,
    "roccio",     init_wt_fdbk,  wt_fdbk,   close_wt_fdbk,
    "ide",        init_wt_ide,   wt_ide,    close_wt_ide,
    "empty",      EMPTY,         EMPTY,       EMPTY,
};
static int num_proc_weight =
    sizeof(proc_weight) / sizeof(proc_weight[0]);

extern int init_form_query(), form_query(), close_form_query();
static PROC_TAB proc_form[] = {
   "form_query", init_form_query,   form_query,    close_form_query,
    "empty",      EMPTY,         EMPTY,       EMPTY,
};
static int num_proc_form =
    sizeof(proc_form) / sizeof(proc_form[0]);


TAB_PROC_TAB proc_feedback[] = {
    "feedback",   TPT_PROC,   NULL, topproc_feedback,  &num_topproc_feedback,
    "expand",     TPT_PROC,   NULL, proc_expand,       &num_proc_expand,
    "occ_info",   TPT_PROC,   NULL, proc_occ_info,     &num_proc_occ_info,
    "weight",     TPT_PROC,   NULL, proc_weight,       &num_proc_weight,
    "form",       TPT_PROC,   NULL, proc_form,         &num_proc_form,
    };

int num_proc_feedback = sizeof (proc_feedback) / sizeof (proc_feedback[0]);




