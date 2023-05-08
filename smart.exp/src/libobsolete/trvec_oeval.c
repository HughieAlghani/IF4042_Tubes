#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/trvec_oeval.c,v 11.0 1992/07/21 18:22:45 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Given ranks for top docs for a query, evaluate the query
 *2 trvec_oeval (tr_vec, eval, inst)
 *3   TR_VEC *tr_vec;
 *3   OLDSM_EVAL *eval;
 *3   int inst;
 *4 init_trvec_oeval (spec, unused)
 *5   "convert.eval.qrels_file"
 *5   "convert.eval.qrels_file.rmode"
 *5   "convert.eval.trace"
 *4 close_trvec_oeval (inst)
 *7 Calculate various evaluation measures for this query.  
 *7 Eval is of type (from sm_eval.h)
 *7 #define ONUM_RP_PTS  21
 *7 #define NUM_CUTOFF  4
 *7 #define CUTOFF_VALUES  {5, 10, 15, 30}
 *7 typedef struct {
 *7     long  qid;                  * query id  (for overall average figures,
 *7                                   this gives number of queries in run) * 
 *7     long num_rel;               * Number of relevant docs * 
 *7     long num_ret;               * Number of retrieved docs * 
 *7     long num_rel_ret;           * Number of relevant retrieved docs * 
 *7     long num_trunc_ret;         * MIN (Number of retrieved docs, poorest
 *7                                   rank of any relevant doc, whether
 *7                                   retrieved of not).
 *7 
 *7     float recall_precis[ONUM_RP_PTS]; *Recall precision at .05 increments * 
 *7     float av_recall_precis;    * average at 3 intermediate points * 
 *7 
 *7     float exact_recall;        * Recall after num_ret docs * 
 *7     float recall_cut[NUM_CUTOFF]; * Recall after cutoff[i] docs * 
 *7 
 *7     float exact_precis;        * Precision after num_ret docs * 
 *7     float precis_cut[NUM_CUTOFF];  * precision after cutoff[i] docs. If
 *7                                        less than cutoff[i] docs retrieved,
 *7                                        then assume an additional 
 *7                                        cutoff[i]-num_ret non-relevant docs
 *7                                        are retrieved. * 
 *7     float exact_trunc_precis;  * Precision after num_trunc_ret docs * 
 *7     float trunc_cut[NUM_CUTOFF];    * precision after cutoff[i] docs or
 *7                                       rank of last rel doc, whichever is
 *7                                       less. If not all rel docs retrieved &
 *7                                       less than cutoff[i] docs retrieved,
 *7                                       then assume an additional 
 *7                                       cutoff[i]-num_ret non-relevant docs
 *7                                       are retrieved. *
 *7 } OLDSM_EVAL;
 *7 
 *7 The number of relevant docs for this query is gotten from qrels_file.
 *7 If qrels file does not exist, then the top docs are assumed to contain
 *7 all relevant docs (ie, exact_recall = 1.0).
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "io.h"
#include "rel_header.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "tr_vec.h"
#include "sm_eval.h"
#include "rr_vec.h"

static char *qrels_file;        /* Gives number of rel docs for each query */
static long qrels_mode;
static SPEC_PARAM spec_args[] = {
    "convert.eval.qrels_file",    getspec_dbfile, (char *) &qrels_file,
    "convert.eval.qrels_file.rmode", getspec_filemode, (char *) &qrels_mode,
    TRACE_PARAM ("convert.trvec_oeval.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

 /* cutoff values for recall precision output */
static int cutoff[NUM_CUTOFF] = CUTOFF_VALUES;

static int compare_did();
static int compare_iter_rank();

static int qrels_fd;

int
init_trvec_oeval (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    qrels_fd = UNDEF;
    if (VALID_FILE (qrels_file) &&
        UNDEF == (qrels_fd = open_rr_vec (qrels_file, qrels_mode)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering/leaving init_trvec_oeval");

    return (0);
}

int
trvec_oeval (tr_vec, eval, inst)
TR_VEC *tr_vec;
OLDSM_EVAL *eval;
int inst;
{
    double recall, precis;    /* current recall value               */
    int   recall_pt = 0;      /* current recall level point no.     */
    /* variables needed for calculating averages, std. dev. */
    int rel_at_cut[NUM_CUTOFF];      /* no. of rel. docs retr within cutoff */
    RR_VEC qrels_vec;
    long i,j;
    int rel_so_far;
    int max_iter;
    int last_rel_ret_rank;

    PRINT_TRACE (2, print_string, "Trace: entering trvec_oeval");

    if (tr_vec == (TR_VEC *) NULL)
        return (UNDEF);

    /* Initialize everything to 0 */
    bzero ((char *) eval, sizeof (OLDSM_EVAL));
    eval->qid = tr_vec->qid;
    for (i = 0; i < NUM_CUTOFF; i++)
        rel_at_cut[i] = 0;

    /* If no relevant docs, then just return */
    if (tr_vec->num_tr == 0) {
        PRINT_TRACE (2, print_string, "Trace: leaving trvec_oeval");
        return (0);
    }

    /* Get number of rel docs from either qrels_file or tr_vec */
    eval->num_rel = 0;
    if (qrels_fd != UNDEF) {
        qrels_vec.qid = tr_vec->qid;
        if (1 == seek_rr_vec (qrels_fd, &qrels_vec) &&
            1 == read_rr_vec (qrels_fd, &qrels_vec))
            eval->num_rel = qrels_vec.num_rr;
    }
    else {
        for (j = 0; j < tr_vec->num_tr; j++) {
            if (tr_vec->tr[j].rel)
                eval->num_rel++;
        }
    }

    /* Evaluate only the docs on the last iteration of tr_vec */
    /* Sort the tr tuples for this query by decreasing iter and 
       increasing rank */
    qsort ((char *) tr_vec->tr,
           (int) tr_vec->num_tr,
           sizeof (TR_TUP),
           compare_iter_rank);

    max_iter = tr_vec->tr[0].iter;
    last_rel_ret_rank = 0;
    for (j = 0; j < tr_vec->num_tr; j++) {
        if (tr_vec->tr[j].iter == max_iter) {
            eval->num_ret++;
            if (tr_vec->tr[j].rel)
                last_rel_ret_rank = j + 1;
        }
        else {
            if (tr_vec->tr[j].rel)
                eval->num_rel--;
        }
    }

    rel_so_far = 0;
    for (j = 0; j < eval->num_ret; j++) {
        if (tr_vec->tr[j].rel) {
            rel_so_far++;
            precis = (float) rel_so_far / (float) (j + 1);
            recall = (float) rel_so_far / (float) eval->num_rel;
            
            /*  Add to any appropriate cutoff limit */
            for (i = 0; i < NUM_CUTOFF; i++) {
                if (j + 1 <= cutoff[i])
                    rel_at_cut[i]++;
            }

            eval->num_rel_ret++;

            recall_pt = recall * (ONUM_RP_PTS - 1) + 0.01;
            if (eval->recall_precis[recall_pt] < precis) {
                eval->recall_precis[recall_pt] = precis;
            }
        }
    }

    /* Need to interpolate precision values */
    for (i = ONUM_RP_PTS - 1; i >= 0; i--) {
        if (eval->recall_precis[i+1] > eval->recall_precis[i])
            eval->recall_precis[i] = eval->recall_precis[i+1];
    }

    /* Calculate other evaluation measures. */
    /* First, average recall-precis of 3 intermediate points */
    eval->av_recall_precis =
        (eval->recall_precis[    (ONUM_RP_PTS / 4) ] +
         eval->recall_precis[2 * (ONUM_RP_PTS / 4) ] +
         eval->recall_precis[3 * (ONUM_RP_PTS / 4) ]) / 3.0;
    
    if (eval->num_rel) {
        for (i = 0; i < NUM_CUTOFF; i++) {
            eval->recall_cut[i] = (double) rel_at_cut[i] / eval->num_rel;
            eval->precis_cut[i] = (double) rel_at_cut[i] / cutoff[i];
            if (rel_so_far == eval->num_rel &&
                last_rel_ret_rank < cutoff[i])
                eval->trunc_cut[i] = (double)rel_at_cut[i] / last_rel_ret_rank;
            else
                eval->trunc_cut[i] = (double)rel_at_cut[i] / cutoff[i];
        }
        eval->exact_recall = (double) eval->num_rel_ret / eval->num_rel;
        eval->exact_precis = (double) eval->num_rel_ret / eval->num_ret;
        if (rel_so_far == eval->num_rel) {
            eval->exact_trunc_precis = (double) eval->num_rel_ret /
                last_rel_ret_rank;
            eval->num_trunc_ret = last_rel_ret_rank;
        }
        else {
            eval->exact_trunc_precis = eval->exact_precis;
            eval->num_trunc_ret = eval->num_ret;
        }
    }

    /* Resort the tr tuples for this query back to increasing did */
    qsort ((char *) tr_vec->tr,
           (int) tr_vec->num_tr,
           sizeof (TR_TUP),
           compare_did);
    
    PRINT_TRACE (2, print_string, "Trace: leaving trvec_oeval");
    return (1);
}


int
close_trvec_oeval (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_trvec_oeval");

    if (VALID_FILE (qrels_file) &&
        UNDEF == close_rr_vec (qrels_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_trvec_oeval");
    return (0);
}


static int
compare_iter_rank (tr1, tr2)
TR_TUP *tr1;
TR_TUP *tr2;
{
    if (tr1->iter > tr2->iter)
        return (-1);
    if (tr1->iter < tr2->iter)
        return (1);
    if (tr1->rank < tr2->rank)
        return (-1);
    if (tr1->rank > tr2->rank)
        return (1);
    return (0);
}

static int
compare_did (tr1, tr2)
TR_TUP *tr1;
TR_TUP *tr2;
{
    if (tr1->did < tr2->did)
        return (-1);
    if (tr1->did > tr2->did)
        return (1);
    return (0);
}
