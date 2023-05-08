#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libproc/proc_print.c,v 11.0 1992/07/21 18:23:08 chrisb Exp $";
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

extern int init_print(), print(), close_print();
extern int init_print_obj_querytext();
static PROC_TAB topproc_print[] = {
    "print",  init_print,     print,     close_print,
};
static int num_topproc_print =
    sizeof(topproc_print) / sizeof(topproc_print[0]);

extern int init_print_obj_doctext(), print_obj_text(), close_print_obj_text();
extern int init_print_obj_querytext();
extern int init_print_obj_vec_dict(),
            print_obj_vec_dict(), close_print_obj_vec_dict();
extern int init_print_obj_vec(), print_obj_vec(), close_print_obj_vec();
extern int init_print_obj_tr_vec(),print_obj_tr_vec(),close_print_obj_tr_vec();
extern int init_print_obj_tr_eval(), print_obj_tr_eval(),
            close_print_obj_tr_eval();
extern int init_print_obj_rr_eval(), print_obj_rr_eval(),
            close_print_obj_rr_eval();
extern int init_print_obj_tr_oeval(), print_obj_tr_oeval(),
            close_print_obj_tr_oeval();
extern int init_print_obj_rr_oeval(), print_obj_rr_oeval(),
            close_print_obj_rr_oeval();
extern int init_print_obj_textloc(), print_obj_textloc(), 
            close_print_obj_textloc();
extern int init_print_obj_rr_vec(),print_obj_rr_vec(),close_print_obj_rr_vec();
extern int init_print_obj_inv(), print_obj_inv(), close_print_obj_inv();
extern int init_print_obj_dict(), print_obj_dict(), close_print_obj_dict();
extern int init_print_obj_proc(), print_obj_proc(), close_print_obj_proc();
extern int init_print_obj_did_nonvalid(), print_obj_did_nonvalid(), 
           close_print_obj_did_nonvalid();
extern int init_print_obj_rel_header(), print_obj_rel_header(), 
            close_print_obj_rel_header();
static PROC_TAB proc_obj[] = {
    "doctext",  init_print_obj_doctext, print_obj_text, close_print_obj_text,
    "querytext",init_print_obj_querytext,print_obj_text,close_print_obj_text,
    "proc",	init_print_obj_proc,	print_obj_proc,	close_print_obj_proc,
    "did_nonvalid",init_print_obj_did_nonvalid,print_obj_did_nonvalid,
                close_print_obj_did_nonvalid,
    "dict",	init_print_obj_dict,	print_obj_dict,	close_print_obj_dict,
    "inv",	init_print_obj_inv,	print_obj_inv,	close_print_obj_inv,
    "rr_eval",	init_print_obj_rr_eval, print_obj_rr_eval,
                close_print_obj_rr_eval,
    "rr_vec",	init_print_obj_rr_vec, print_obj_rr_vec,close_print_obj_rr_vec,
    "textloc",  init_print_obj_textloc,print_obj_textloc,
                close_print_obj_textloc,
    "tr_eval",	init_print_obj_tr_eval, print_obj_tr_eval,
                close_print_obj_tr_eval,
    "tr_vec",	init_print_obj_tr_vec, print_obj_tr_vec,close_print_obj_tr_vec,
    "vec",	init_print_obj_vec,	print_obj_vec,	close_print_obj_vec,
    "vec_dict", init_print_obj_vec_dict,print_obj_vec_dict,
                close_print_obj_vec_dict,
    "rr_oeval",	init_print_obj_rr_oeval, print_obj_rr_oeval,
                close_print_obj_rr_oeval,
    "tr_oeval",	init_print_obj_tr_oeval, print_obj_tr_oeval,
                close_print_obj_tr_oeval,
    "rel_header",init_print_obj_rel_header, print_obj_rel_header,
	        close_print_obj_rel_header,
};
static int num_proc_obj =
    sizeof(proc_obj) / sizeof(proc_obj[0]);

extern int init_print_text_pp(), print_text_pp(), close_print_text_pp();
extern int init_print_text_raw(), print_text_raw(), close_print_text_raw();
extern int init_print_text_filter(), print_text_filter(), 
       close_print_text_filter();
extern int init_print_text_form(), print_text_form(), close_print_text_form();
static PROC_TAB proc_indivtext[] = {
    "text_pp", init_print_text_pp,   print_text_pp,    close_print_text_pp,
    "text_raw",init_print_text_raw,  print_text_raw,   close_print_text_raw,
    "text_filter",init_print_text_filter,  print_text_filter,   
                close_print_text_filter,
    "text_form",init_print_text_form,print_text_form,  close_print_text_form,
    };
static int num_proc_indivtext = sizeof (proc_indivtext) / sizeof (proc_indivtext[0]);


TAB_PROC_TAB proc_print[] = {
   "top",       TPT_PROC, NULL, topproc_print,     &num_topproc_print,
   "obj",       TPT_PROC, NULL, proc_obj,          &num_proc_obj,
   "indivtext",	TPT_PROC, NULL,	proc_indivtext,    &num_proc_indivtext,
    };

int num_proc_print = sizeof (proc_print) / sizeof (proc_print[0]);



