#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/show_exp.c,v 11.0 1992/07/21 18:21:49 chrisb Exp $";
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
#include "spec.h"
#include "inter.h"
#include "rr_vec.h"
#include "sm_eval.h"

static char *tr_file;
static long tr_mode;
static char *rr_file;
static long rr_mode;

static SPEC_PARAM spec_args[] = {
    "retrieve.tr_file",           getspec_dbfile,   (char *) &tr_file,
    "retrieve.tr_file.rmode",     getspec_filemode, (char *) &tr_mode,
    "retrieve.rr_file",           getspec_dbfile,   (char *) &rr_file,
    "retrieve.rr_file.rmode",     getspec_filemode, (char *) &rr_mode,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_init = 0;

static int tr_fd, rr_fd;
static int rrvec_smeval_inst;

int
init_show_exp (spec, unused)
SPEC *spec;
char *unused;
{
    /* Only initialize the very first time this is called. */
    if (num_init++)
        return (0);

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    /* Open files to be examined (Note that fd == UNDEF upon error) */
    tr_fd = rr_fd = UNDEF;
    if (VALID_FILE (tr_file))
        tr_fd = open_tr_vec (tr_file, tr_mode);
    if (VALID_FILE (rr_file))
        rr_fd = open_rr_vec (rr_file, rr_mode);

    if (UNDEF == (rrvec_smeval_inst = init_rrvec_smeval (spec, (char *) NULL)))
        return (UNDEF);
    return (1);
}


int
close_show_exp (inst)
int inst;
{
    /* Only close after all instantiations done */
    if (--num_init)
        return (0);

    /* Close files if open */
    if (tr_fd != UNDEF && UNDEF == close_tr_vec (tr_fd))
        return (UNDEF);
    if (rr_fd != UNDEF && UNDEF == close_rr_vec (rr_fd))
        return (UNDEF);
    if (UNDEF == close_rrvec_smeval (rrvec_smeval_inst))
        return (UNDEF);

    return (1);
}


int
show_exp_rr (is, unused)
INTER_STATE *is;
char *unused;
{
    RR_VEC rr_vec;

    if (is->retrieval.query->qid == UNDEF) {
        if (UNDEF == add_buf_string ("No query specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    if (rr_fd == UNDEF) {
        if (UNDEF == add_buf_string ("rr file not found\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    
    rr_vec.qid = is->retrieval.query->qid;
    if (1 != seek_rr_vec (rr_fd, &rr_vec) ||
        1 != read_rr_vec (rr_fd, &rr_vec)) {
        if (UNDEF == add_buf_string ("No relevant docs found\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (UNDEF == add_buf_string ("Qid\tDid\tRank\tSim\n", &is->output_buf))
        return (UNDEF);
    print_rr_vec (&rr_vec, &is->output_buf);

    return (1);
}

int
show_exp_tr (is, unused)
INTER_STATE *is;
char *unused;
{
    TR_VEC tr_vec;

    if (is->retrieval.query->qid == UNDEF) {
        if (UNDEF == add_buf_string ("No query specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    if (tr_fd == UNDEF) {
        if (UNDEF == add_buf_string ("tr file not found\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    tr_vec.qid = is->retrieval.query->qid;
    if (1 != seek_tr_vec (tr_fd, &tr_vec) ||
        1 != read_tr_vec (tr_fd, &tr_vec)) {
        if (UNDEF == add_buf_string ("No top docs found\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (UNDEF == add_buf_string ("Qid\tDid\tRank\tRel\tAction\tIter\tSim\n",
                                 &is->output_buf))
        return (UNDEF);
    print_tr_vec (&tr_vec, &is->output_buf);
    
    return (1);
}

int
show_exp_eval (is, unused)
INTER_STATE *is;
char *unused;
{
    SM_EVAL eval;
    RR_VEC rr_vec;

    if (is->retrieval.query->qid == UNDEF) {
        if (UNDEF == add_buf_string ("No query specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    if (rr_fd == UNDEF) {
        if (UNDEF == add_buf_string ("Rr file (for eval) not found\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    eval.qid = is->retrieval.query->qid;
    if (1 != seek_rr_vec (rr_fd, &rr_vec) ||
        1 != read_rr_vec (rr_fd, &rr_vec)) {
        if (UNDEF == add_buf_string ("Rr records not found for query\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }
    if (UNDEF == rrvec_smeval (&rr_vec, &eval, rrvec_smeval_inst))
        return (UNDEF);

    print_smeval (&eval, &is->output_buf);
    
    return (1);
}

int
show_exp_rclprn (is, unused)
INTER_STATE *is;
char *unused;
{
    print_rclprn (&is->spec_list, &is->output_buf);

    return (1);
}

int
show_exp_tr_eval (is, unused)
INTER_STATE *is;
char *unused;
{
    print_tr_eval (&is->spec_list, &is->output_buf);
    return (1);
}

int
show_exp_rr_eval (is, unused)
INTER_STATE *is;
char *unused;
{
    print_rr_eval (&is->spec_list, &is->output_buf);
    return (1);
}

int
show_exp_ind_rr (is, unused)
INTER_STATE *is;
char *unused;
{
    print_ind_rr (&is->spec_list, &is->output_buf);
    return (1);
}

int
show_exp_ind_tr (is, unused)
INTER_STATE *is;
char *unused;
{
    print_ind_tr (&is->spec_list, &is->output_buf);
    return (1);
}


