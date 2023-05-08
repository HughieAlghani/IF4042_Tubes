#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libproc/proc_convert.c,v 11.0 1992/07/21 18:22:23 chrisb Exp $";
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

static PROC_TAB topproc_convert[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
};
static int num_topproc_convert =
    sizeof(topproc_convert) / sizeof(topproc_convert[0]);

extern int init_trec_tr_obj(), trec_tr_obj(), close_trec_tr_obj();
extern int init_tr_trec_obj(), tr_trec_obj(), close_tr_trec_obj();
extern int init_trec_qrels_obj(), trec_qrels_obj(), close_trec_qrels_obj();
extern int init_textloc_partvec_obj(), textloc_partvec_obj(),
           close_textloc_partvec_obj();
static PROC_TAB proc_obj[] = {
    "trec_tr",        init_trec_tr_obj,   trec_tr_obj,    close_trec_tr_obj,
    "tr_trec",        init_tr_trec_obj,   tr_trec_obj,    close_tr_trec_obj,
    "trec_qrels",     init_trec_qrels_obj,trec_qrels_obj, close_trec_qrels_obj,
    "textloc_partvec", init_textloc_partvec_obj, textloc_partvec_obj,
                       close_textloc_partvec_obj,
    };
static int num_proc_obj = sizeof (proc_obj) / sizeof (proc_obj[0]);

extern int init_contok_dict_noinfo(), contok_dict_noinfo(),
    close_contok_dict_noinfo();
extern int init_pp_veclist_prefix(), pp_veclist_prefix(),
    close_pp_veclist_prefix();
extern int init_pp_partvec(), pp_partvec(), close_pp_partvec();
static PROC_TAB proc_tup[] = {
    "contok_dict_noinfo",     init_contok_dict_noinfo,   contok_dict_noinfo,
              close_contok_dict_noinfo,
    "pp_veclist_prefix",      init_pp_veclist_prefix, pp_veclist_prefix,
              close_pp_veclist_prefix,
    "pp_partvec", init_pp_partvec, pp_partvec, close_pp_partvec,
    };
static int num_proc_tup = sizeof (proc_tup) / sizeof (proc_tup[0]);


static PROC_TAB proc_wt[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
    };
static int num_proc_wt = sizeof (proc_wt) / sizeof (proc_wt[0]);

static PROC_TAB proc_wt_ctype[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
    };
static int num_proc_wt_ctype = sizeof (proc_wt_ctype) /
         sizeof (proc_wt_ctype[0]);

static PROC_TAB proc_wt_tf[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
};
static int num_proc_wt_tf = sizeof (proc_wt_tf) / sizeof (proc_wt_tf[0]);

static PROC_TAB proc_wt_idf[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
};
static int num_proc_wt_idf = sizeof (proc_wt_idf) / sizeof (proc_wt_idf[0]);

static PROC_TAB proc_wt_norm[] = {
    "empty",            INIT_EMPTY,     EMPTY,          CLOSE_EMPTY
};
static int num_proc_wt_norm = sizeof (proc_wt_norm) / sizeof (proc_wt_norm[0]);


TAB_PROC_TAB lproc_convert[] = {
    "convert",  TPT_PROC, NULL, topproc_convert, &num_topproc_convert,
    "obj",	TPT_PROC, NULL,	proc_obj,    	&num_proc_obj,
    "tup",	TPT_PROC, NULL,	proc_tup,    	&num_proc_tup,
    "weight",	TPT_PROC, NULL,	proc_wt, 	&num_proc_wt,
    "wt_ctype",	TPT_PROC, NULL,	proc_wt_ctype, 	&num_proc_wt_ctype,
    "wt_tf",	TPT_PROC, NULL,	proc_wt_tf, 	&num_proc_wt_tf,
    "wt_idf",	TPT_PROC, NULL,	proc_wt_idf, 	&num_proc_wt_idf,
    "wt_norm",	TPT_PROC, NULL,	proc_wt_norm, 	&num_proc_wt_norm,
    };
int num_lproc_convert = sizeof (lproc_convert) / sizeof (lproc_convert[0]);
