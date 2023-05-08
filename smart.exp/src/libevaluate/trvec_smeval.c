#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libevaluate/trvec_smeval.c,v 11.0 1992/07/21 18:20:35 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Given ranks for top docs for a query, evaluate the query
 *2 trvec_smeval (tr_vec, eval, inst)
 *3   TR_VEC *tr_vec;
 *3   SM_EVAL *eval;
 *3   int inst;
 *4 init_trvec_smeval (spec, unused)
 *5   "convert.eval.qrels_file"
 *5   "convert.eval.qrels_file.rmode"
 *5   "convert.eval.trace"
 *4 close_trvec_smeval (inst)
 *7 Calculate various evaluation measures for this query.  
 *7 Eval is of type (from sm_eval.h)
 *7 #define NUM_RP_PTS  11
 *7 #define THREE_PTS {2, 5, 8}    
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
 *7     float recall_precis[NUM_RP_PTS]; *Recall precision at 0.1 increments * 
 *7     float av_recall_precis;         * average at all points *
 *7     float three_av_recall_precis;   * average at 3 intermediate points *
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
 *7 } SM_EVAL;
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
    TRACE_PARAM ("convert.trvec_smeval.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

 /* cutoff values for recall precision output */
static int cutoff[NUM_CUTOFF] = CUTOFF_VALUES;
static int three_pts[3] = THREE_PTS;

static int compare_iter_rank();
static int copy_tr_vec();

static int qrels_fd;

int
init_trvec_smeval (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    qrels_fd = UNDEF;
    if (VALID_FILE (qrels_file) &&
        UNDEF == (qrels_fd = open_rr_vec (qrels_file, qrels_mode)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering/leaving init_trvec_smeval");

    return (0);
}

int
trvec_smeval (tr_vec, eval, inst)
TR_VEC *tr_vec;
SM_EVAL *eval;
int inst;
{
    double recall, precis;    /* current recall value               */
    int   recall_pt = 0;      /* current recall level point no.     */
    /* variables needed for calculating averages, std. dev. */
    int rel_at_cut[NUM_CUTOFF];      /* no. of rel. docs retr within cutoff */
    RR_VEC qrels_vec;
    long i,j;
    long rel_so_far;
    long max_iter;
    long last_rel_ret_rank;
    TR_VEC new_tr_vec;        /* Modifiable copy of tr_vec */

    PRINT_TRACE (2, print_string, "Trace: entering trvec_smeval");

    if (tr_vec == (TR_VEC *) NULL)
        return (UNDEF);

    if (UNDEF == copy_tr_vec (tr_vec, &new_tr_vec))
        return (UNDEF);

    /* Initialize everything to 0 */
    bzero ((char *) eval, sizeof (SM_EVAL));
    eval->qid = new_tr_vec.qid;
    for (i = 0; i < NUM_CUTOFF; i++)
        rel_at_cut[i] = 0;

    /* If no relevant docs, then just return */
    if (new_tr_vec.num_tr == 0) {
        PRINT_TRACE (2, print_string, "Trace: leaving trvec_smeval");
        return (0);
    }

    /* Get number of rel docs from either qrels_file or new_tr_vec */
    eval->num_rel = 0;
    if (qrels_fd != UNDEF) {
        qrels_vec.qid = new_tr_vec.qid;
        if (1 == seek_rr_vec (qrels_fd, &qrels_vec) &&
            1 == read_rr_vec (qrels_fd, &qrels_vec))
            eval->num_rel = qrels_vec.num_rr;
    }
    else {
        for (j = 0; j < new_tr_vec.num_tr; j++) {
            if (new_tr_vec.tr[j].rel)
                eval->num_rel++;
        }
    }

    /* Evaluate only the docs on the last iteration of new_tr_vec */
    /* Sort the tr tuples for this query by decreasing iter and 
       increasing rank */
    qsort ((char *) new_tr_vec.tr,
           (int) new_tr_vec.num_tr,
           sizeof (TR_TUP),
           compare_iter_rank);

    max_iter = new_tr_vec.tr[0].iter;
    last_rel_ret_rank = 0;
    for (j = 0; j < new_tr_vec.num_tr; j++) {
        if (new_tr_vec.tr[j].iter == max_iter) {
            eval->num_ret++;
            if (new_tr_vec.tr[j].rel)
                last_rel_ret_rank = j + 1;
        }
        else {
            if (new_tr_vec.tr[j].rel)
                eval->num_rel--;
        }
    }

    rel_so_far = 0;
    for (j = 0; j < eval->num_ret; j++) {
        if (new_tr_vec.tr[j].rel) {
            rel_so_far++;
            precis = (float) rel_so_far / (float) (j + 1);
            recall = (float) rel_so_far / (float) eval->num_rel;
            
            /*  Add to any appropriate cutoff limit */
            for (i = 0; i < NUM_CUTOFF; i++) {
                if (j + 1 <= cutoff[i])
                    rel_at_cut[i]++;
            }

            eval->num_rel_ret++;

            recall_pt = recall * (NUM_RP_PTS - 1) + 0.01;
            if (eval->recall_precis[recall_pt] < precis) {
                eval->recall_precis[recall_pt] = precis;
            }
        }
    }

    /* Need to interpolate precision values */
    for (i = NUM_RP_PTS - 1; i >= 0; i--) {
        if (eval->recall_precis[i+1] > eval->recall_precis[i])
            eval->recall_precis[i] = eval->recall_precis[i+1];
    }

    /* Calculate other evaluation measures. */
    /* First, average recall-precis of 3 intermediate points */
    eval->three_av_recall_precis =
        (eval->recall_precis[three_pts[0]] +
         eval->recall_precis[three_pts[1]] +
         eval->recall_precis[three_pts[2]]) / 3.0;

    for (i = 0; i < NUM_RP_PTS; i++) {
        eval->av_recall_precis += eval->recall_precis[i];
    }
    eval->av_recall_precis /= NUM_RP_PTS;

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

    PRINT_TRACE (2, print_string, "Trace: leaving trvec_smeval");
    return (1);
}


int
close_trvec_smeval (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_trvec_smeval");

    if (VALID_FILE (qrels_file) &&
        UNDEF == close_rr_vec (qrels_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_trvec_smeval");
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

static TR_TUP *save_tr_tup;
static long num_save_tr_tup = 0;

static int 
copy_tr_vec (in_tr, out_tr)
TR_VEC *in_tr;
TR_VEC *out_tr;
{
    if (in_tr->num_tr > num_save_tr_tup) {
        if (num_save_tr_tup != 0)
            (void) free ((char *) save_tr_tup);
        if (NULL == (save_tr_tup = (TR_TUP *)
                     malloc ((unsigned) in_tr->num_tr * sizeof (TR_TUP))))
            return (UNDEF);
        num_save_tr_tup = in_tr->num_tr;
    }

    out_tr->qid = in_tr->qid;
    out_tr->num_tr = in_tr->num_tr;
    if (in_tr->num_tr == 0)
        return (0);

    out_tr->tr = save_tr_tup;

    (void) bcopy ((char *) in_tr->tr,
                  (char *) out_tr->tr,
                  (int) in_tr->num_tr * sizeof (TR_TUP));
    return (1);
}
