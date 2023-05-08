#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/tr_oeval.c,v 11.0 1992/07/21 18:22:45 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Given a tr file, evaluate it, returning the evaluation in eval
 *2 tr_oeval (tr_file, eval, inst)
 *3   char *tr_file;
 *3   OLDSM_EVAL *eval;
 *3   int inst;
 *4 init_tr_oeval (spec, unused)
 *5   "eval.tr_file"
 *5   "eval.tr_file.rmode"
 *5   "eval.trace"
 *4 close_tr_oeval (inst)
 *7 Evaluate the given tr_file, returning the average over all queries of
 *7 each query's evaluation.  Eval->qid will contain the number of queries
 *7 evaluated. Tr_file is taken from the argument tr_file if
 *7 that is valid, else from the spec parameter eval.tr_file.
 *7 Return 1 if successful, 0 if no queries were evaluated, and UNDEF otherwise
 *8 Call trvec_oeval for each query in tr_file, and simply average results.
 *9 Note: Only the max iteration over all queries is averaged.  Thus, if query
 *9 1 had one iteration of feedback, and query 2 had two iterations of
 *9 feedback, query 1 will not be included in the final average (or counted
 *9 in eval->qid).
 *9 WARNING: works with old evaluation format
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
#include "tr_vec.h"
#include "sm_eval.h"

static char *default_tr_file;
static long tr_mode;

static SPEC_PARAM spec_args[] = {
    "eval.tr_file",           getspec_dbfile,   (char *) &default_tr_file,
    "eval.tr_file.rmode",     getspec_filemode, (char *) &tr_mode,
    TRACE_PARAM ("eval.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

static int trvec_oeval_inst;


int
init_tr_oeval (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_tr_oeval");
    if (UNDEF == (trvec_oeval_inst = init_trvec_oeval (spec, (char *) NULL)))
        return (UNDEF);
    PRINT_TRACE (2, print_string, "Trace: leaving init_tr_oeval");

    return (0);
}

int
tr_oeval (in_tr_file, eval, inst)
char *in_tr_file;
OLDSM_EVAL *eval;
int inst;
{
    char *tr_file;
    long i;
    TR_VEC tr_vec;
    int tr_fd;
    int max_iter;
    int max_iter_achieved;
    OLDSM_EVAL query_eval;

    PRINT_TRACE (2, print_string, "Trace: entering tr_oeval");
    PRINT_TRACE (6, print_string, in_tr_file);

    tr_file = in_tr_file;
    if (! VALID_FILE (tr_file))
        tr_file = default_tr_file;

    if (UNDEF == (tr_fd = open_tr_vec (tr_file, tr_mode)))
        return (UNDEF);

    (void) bzero ((char *) eval, sizeof (OLDSM_EVAL));
    max_iter = 0;
    while (1 == read_tr_vec (tr_fd, &tr_vec)) {
        /* Check that max_iter has not been exceeded.  If it has, then have
           to throw away all previous results.
           Also check to see that max_iter has been achieved.  If not, then
           no docs were retrieved for this query on this iteration */
        max_iter_achieved = 0;
        for (i = 0; i < tr_vec.num_tr; i++) {
            if (tr_vec.tr[i].iter > max_iter) {
                (void) bzero ((char *) eval, sizeof (OLDSM_EVAL));
                max_iter = tr_vec.tr[i].iter;
            }
            if (tr_vec.tr[i].iter == max_iter) {
                max_iter_achieved++;
            }
        }
        if (max_iter_achieved == 0)
            continue;

        /* Evaluate this query */
        if (UNDEF == trvec_oeval (&tr_vec, &query_eval, trvec_oeval_inst))
            return (UNDEF);
        if (query_eval.num_ret > 0) {
            eval->qid++;
            eval->num_rel       += query_eval.num_rel;
            eval->num_ret       += query_eval.num_ret;
            eval->num_rel_ret   += query_eval.num_rel_ret;
            eval->num_trunc_ret += query_eval.num_trunc_ret;
            eval->av_recall_precis += query_eval.av_recall_precis;
            eval->exact_recall  += query_eval.exact_recall;
            eval->exact_precis  += query_eval.exact_precis;
            eval->exact_trunc_precis += query_eval.exact_trunc_precis;
            for (i = 0; i < ONUM_RP_PTS; i++)
                eval->recall_precis[i] += query_eval.recall_precis[i];
            for (i = 0; i < NUM_CUTOFF; i++) {
                eval->recall_cut[i] += query_eval.recall_cut[i];
                eval->precis_cut[i] += query_eval.precis_cut[i];
                eval->trunc_cut[i] += query_eval.trunc_cut[i];
            }
        }
    }

    /* Calculate averages (for those eval fields returning averages) */
    if (eval->qid > 0) {
        eval->av_recall_precis /= (float) eval->qid;
        eval->exact_recall /= (float) eval->qid;
        eval->exact_precis /= (float) eval->qid;
        eval->exact_trunc_precis /= (float) eval->qid;
        for (i = 0; i < ONUM_RP_PTS; i++)
            eval->recall_precis[i] /= (float) eval->qid;
        for (i = 0; i < NUM_CUTOFF; i++) {
            eval->recall_cut[i] /= (float) eval->qid;
            eval->precis_cut[i] /= (float) eval->qid;
            eval->trunc_cut[i] /= (float) eval->qid;
        }
    }

    if (UNDEF == close_tr_vec (tr_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving tr_oeval");

    if (eval->qid)
        return (1);
    return (0);
}

int
close_tr_oeval(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_tr_oeval");
    if (UNDEF == close_trvec_oeval (trvec_oeval_inst))
        return (UNDEF);
    PRINT_TRACE (2, print_string, "Trace: leaving close_tr_oeval");
    return (0);
}

