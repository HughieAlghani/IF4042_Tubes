#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libproc/proc_print.c,v 11.0 1992/07/21 18:22:23 chrisb Exp $";
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

static PROC_TAB topproc_print[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
};
static int num_topproc_print =
    sizeof(topproc_print) / sizeof(topproc_print[0]);


extern int init_print_obj_dict_noinfo(), print_obj_dict_noinfo(), 
    close_print_obj_dict_noinfo();
extern int init_print_obj_partvec(), print_obj_partvec(),
           close_print_obj_partvec();
static PROC_TAB proc_obj[] = {
    "dict_noinfo",	init_print_obj_dict_noinfo,	print_obj_dict_noinfo,
	close_print_obj_dict_noinfo,
    "partvec",	init_print_obj_partvec, print_obj_partvec,
           close_print_obj_partvec,
};
static int num_proc_obj =
    sizeof(proc_obj) / sizeof(proc_obj[0]);


static PROC_TAB proc_indivtext[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
    };
static int num_proc_indivtext = sizeof (proc_indivtext) / sizeof (proc_indivtext[0]);


TAB_PROC_TAB lproc_print[] = {
   "top",       TPT_PROC, NULL, topproc_print,     &num_topproc_print,
   "obj",       TPT_PROC, NULL, proc_obj,          &num_proc_obj,
   "indivtext",	TPT_PROC, NULL,	proc_indivtext,    &num_proc_indivtext,
    };

int num_lproc_print = sizeof (lproc_print) / sizeof (lproc_print[0]);



