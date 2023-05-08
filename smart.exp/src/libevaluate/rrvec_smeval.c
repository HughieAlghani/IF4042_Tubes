#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libevaluate/rrvec_smeval.c,v 11.0 1992/07/21 18:20:34 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Given ranks for relevant docs for a query, evaluate the query
 *2 rrvec_smeval (rr_vec, eval, inst)
 *3   RR_VEC *rr_vec;
 *3   SM_EVAL *eval;
 *3   int inst;
 *4 init_rrvec_smeval (spec, unused)
 *5   "convert.eval.num_wanted"
 *5   "convert.eval.trace"
 *4 close_rrvec_smeval (inst)
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
#include "rr_vec.h"
#include "sm_eval.h"

static long num_wanted;           /* Number of retrieved docs for each query */
static SPEC_PARAM spec_args[] = {
    "convert.eval.num_wanted",    getspec_long, (char *) &num_wanted,
    TRACE_PARAM ("convert.eval.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

 /* cutoff values for recall precision output */
static int cutoff[NUM_CUTOFF] = CUTOFF_VALUES;
static int three_pts[3] = THREE_PTS;

static int compare_rank();
static int copy_rr_vec();

int
init_rrvec_smeval (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering/leaving init_rrvec_smeval");

    return (0);
}

int
rrvec_smeval (rr_vec, eval, inst)
RR_VEC *rr_vec;
SM_EVAL *eval;
int inst;
{
    double recall, precis;    /* current recall value               */
    int   recall_pt = 0;      /* current recall level point no.     */
    /* variables needed for calculating averages, std. dev. */
    int rel_at_cut[NUM_CUTOFF];      /* no. of rel. docs retr within cutoff */

    RR_VEC new_rr_vec;        /* Modifiable copy of rr_vec */
    long i,j;

    PRINT_TRACE (2, print_string, "Trace: entering rrvec_smeval");

    if (rr_vec == (RR_VEC *) NULL)
        return (UNDEF);
    if (UNDEF == copy_rr_vec (rr_vec, &new_rr_vec))
        return (UNDEF);

    /* Initialize everything to 0 */
    bzero ((char *) eval, sizeof (SM_EVAL));
    eval->qid = new_rr_vec.qid;
    for (i = 0; i < NUM_CUTOFF; i++)
        rel_at_cut[i] = 0;

    /* If no relevant docs, then just return */
    if (new_rr_vec.num_rr == 0) {
        PRINT_TRACE (2, print_string, "Trace: leaving rrvec_smeval");
        return (0);
    }

    /* Sort the rr tuples for this query by increasing rank */
    qsort ((char *) new_rr_vec.rr,
           (int) new_rr_vec.num_rr,
           sizeof (RR_TUP),
           compare_rank);

    eval->num_ret = num_wanted;
    eval->num_rel = new_rr_vec.num_rr;
    eval->num_trunc_ret = MIN (num_wanted, new_rr_vec.rr[new_rr_vec.num_rr-1].rank);

    for (j = 0; j < new_rr_vec.num_rr; j++) {
        recall = (float) (j + 1) / new_rr_vec.num_rr;
        precis = (float) (j + 1) / new_rr_vec.rr[j].rank;

        /*  Add to any appropriate cutoff limit */
        for (i = 0; i < NUM_CUTOFF; i++) {
            if (new_rr_vec.rr[j].rank <= cutoff[i])
                rel_at_cut[i]++;
        }

        if (new_rr_vec.rr[j].rank <= num_wanted)
            eval->num_rel_ret++;

        recall_pt = recall * (NUM_RP_PTS - 1) + 0.01;
        if (eval->recall_precis[recall_pt] < precis) {
            eval->recall_precis[recall_pt] = precis;
        }
    }

    /* Need to interpolate precision values */
    for (i = recall_pt-1; i >= 0; i--) {
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

    for (i = 0; i < NUM_CUTOFF; i++) {
        eval->recall_cut[i] = (double) rel_at_cut[i] / eval->num_rel;
        eval->precis_cut[i] = (double) rel_at_cut[i] / cutoff[i];
        if (new_rr_vec.rr[new_rr_vec.num_rr-1].rank < cutoff[i])
            eval->trunc_cut[i] = (double) rel_at_cut[i] /
                new_rr_vec.rr[new_rr_vec.num_rr-1].rank;
        else
            eval->trunc_cut[i] = (double) rel_at_cut[i] / cutoff[i];
    }

    if (num_wanted > 0) {
        eval->exact_recall = (double) eval->num_rel_ret / eval->num_rel;
        eval->exact_precis = (double) eval->num_rel_ret / eval->num_ret;
        eval->exact_trunc_precis = (double) eval->num_rel_ret /
            eval->num_trunc_ret;
    }

    PRINT_TRACE (2, print_string, "Trace: leaving rrvec_smeval");
    return (1);
}


int
close_rrvec_smeval (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_rrvec_smeval");

    PRINT_TRACE (2, print_string, "Trace: leaving close_rrvec_smeval");
    return (0);
}


static int
compare_rank (rr1, rr2)
RR_TUP *rr1;
RR_TUP *rr2;
{
    if (rr1->rank < rr2->rank)
        return (-1);
    if (rr1->rank > rr2->rank)
        return (1);
    return (0);
}


static RR_TUP *save_rr_tup;
static int num_save_rr_tup = 0;

static int 
copy_rr_vec (in_rr, out_rr)
RR_VEC *in_rr;
RR_VEC *out_rr;
{
    if (in_rr->num_rr > num_save_rr_tup) {
        if (num_save_rr_tup != 0)
            (void) free ((char *) save_rr_tup);
        if (NULL == (save_rr_tup = (RR_TUP *)
                     malloc ((unsigned) in_rr->num_rr * sizeof (RR_TUP))))
            return (UNDEF);
        num_save_rr_tup = in_rr->num_rr;
    }

    out_rr->qid = in_rr->qid;
    out_rr->num_rr = in_rr->num_rr;
    if (in_rr->num_rr == 0)
        return (0);

    out_rr->rr = save_rr_tup;

    (void) bcopy ((char *) in_rr->rr,
                  (char *) out_rr->rr,
                  (int) in_rr->num_rr * sizeof (RR_TUP));
    return (1);
}

