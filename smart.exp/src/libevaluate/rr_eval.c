#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libevaluate/rr_eval.c,v 11.0 1992/07/21 18:20:32 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Given a rr file, evaluate it, returning the evaluation in eval
 *2 rr_eval (rr_file, eval, inst)
 *3   char *rr_file;
 *3   SM_EVAL *eval;
 *3   int inst;
 *4 init_rr_eval (spec, unused)
 *5   "eval.rr_file"
 *5   "eval.rr_file.rmode"
 *5   "eval.trace"
 *4 close_rr_eval (inst)
 *7 Evaluate the given rr_file, returning the average over all queries of
 *7 each query's evaluation.  Eval->qid will contain the number of queries
 *7 evaluated. Rr_file is taken from the argument rr_file if
 *7 that is valid, else from the spec parameter eval.rr_file.
 *7 Return 1 if successful, 0 if no queries were evaluated, and UNDEF otherwise
 *8 Call rrvec_smeval for each query in rr_file, and simply average results.
 *9 Note: Only the max iteration over all queries is averaged.  Thus, if query
 *9 1 had one iteration of feedback, and query 2 had two iterations of
 *9 feedback, query 1 will not be included in the final average (or counted
 *9 in eval->qid).
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "io.h"
#include "inst.h"
#include "proc.h"
#include "rr_vec.h"
#include "sm_eval.h"

static char *default_rr_file;
static long rr_mode;

static SPEC_PARAM spec_args[] = {
    "eval.rr_file",           getspec_dbfile,   (char *) &default_rr_file,
    "eval.rr_file.rmode",     getspec_filemode, (char *) &rr_mode,
    TRACE_PARAM ("eval.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

static int rrvec_smeval_inst;

int
init_rr_eval (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_rr_eval");

    if (UNDEF == (rrvec_smeval_inst = init_rrvec_smeval (spec, (char *) NULL)))
         return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_rr_eval");

    return (0);
}

int
rr_eval (in_rr_file, eval, inst)
char *in_rr_file;
SM_EVAL *eval;
int inst;
{
    char *rr_file;
    long i;
    RR_VEC rr_vec;
    int rr_fd;
    SM_EVAL query_eval;

    PRINT_TRACE (2, print_string, "Trace: entering rr_eval");
    PRINT_TRACE (6, print_string, in_rr_file);

    rr_file = in_rr_file;
    if (! VALID_FILE (rr_file))
        rr_file = default_rr_file;
    if (! VALID_FILE (rr_file))
        return (UNDEF);

    if (UNDEF == (rr_fd = open_rr_vec (rr_file, rr_mode)))
        return (UNDEF);

    (void) bzero ((char *) eval, sizeof (SM_EVAL));

    while (1 == read_rr_vec (rr_fd, &rr_vec)) {
        /* Evaluate this query */
        if (UNDEF == rrvec_smeval (&rr_vec, &query_eval, rrvec_smeval_inst))
            return (UNDEF);

        /* Add results to previous results */
        eval->qid++;
        eval->num_rel       += query_eval.num_rel;
        eval->num_ret       += query_eval.num_ret;
        eval->num_rel_ret   += query_eval.num_rel_ret;
        eval->num_trunc_ret += query_eval.num_trunc_ret;
        eval->av_recall_precis += query_eval.av_recall_precis;
        eval->three_av_recall_precis += query_eval.three_av_recall_precis;
        eval->exact_recall  += query_eval.exact_recall;
        eval->exact_precis  += query_eval.exact_precis;
        eval->exact_trunc_precis += query_eval.exact_trunc_precis;
        for (i = 0; i < NUM_RP_PTS; i++)
            eval->recall_precis[i] += query_eval.recall_precis[i];
        for (i = 0; i < NUM_CUTOFF; i++) {
            eval->recall_cut[i] += query_eval.recall_cut[i];
            eval->precis_cut[i] += query_eval.precis_cut[i];
            eval->trunc_cut[i] += query_eval.trunc_cut[i];
        }
    }

    /* Calculate averages (for those eval fields returning averages) */
    if (eval->qid > 0) {
        eval->av_recall_precis /= (float) eval->qid;
        eval->three_av_recall_precis /= (float) eval->qid;
        eval->exact_recall /= (float) eval->qid;
        eval->exact_precis /= (float) eval->qid;
        eval->exact_trunc_precis /= (float) eval->qid;
        for (i = 0; i < NUM_RP_PTS; i++)
            eval->recall_precis[i] /= (float) eval->qid;
        for (i = 0; i < NUM_CUTOFF; i++) {
            eval->recall_cut[i] /= (float) eval->qid;
            eval->precis_cut[i] /= (float) eval->qid;
            eval->trunc_cut[i]  /= (float) eval->qid;
        }
    }

    if (UNDEF == close_rr_vec (rr_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving rr_eval");

    if (eval->qid)
        return (1);
    return (0);
}

int
close_rr_eval(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_rr_eval");
    if (UNDEF == close_rrvec_smeval (rrvec_smeval_inst))
        return (UNDEF);
    PRINT_TRACE (2, print_string, "Trace: leaving close_rr_eval");
    return (0);
}

