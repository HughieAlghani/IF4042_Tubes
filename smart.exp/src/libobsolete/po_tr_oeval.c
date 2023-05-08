#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/po_tr_oeval.c,v 11.0 1992/07/21 18:22:59 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "spec.h"
#include "buf.h"
#include "sm_eval.h"

static char *run_name;
static char *tr_file;
static char *rr_file;
static long run_num_queries;

static SPEC_PARAM spec_args[] = {
    "print.run_name",        getspec_string, (char *) &run_name,
    "eval.tr_file",          getspec_dbfile, (char *) &tr_file,
    "eval.rr_file",          getspec_dbfile, (char *) &rr_file,
    "eval.num_queries",      getspec_long,   (char *) &run_num_queries,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_BUF internal_output = {0, 0, (char *) 0};

static long cutoff[] = CUTOFF_VALUES;

static void print_eval_list();
static void redo_eval();


void
print_tr_oeval (spec_list, output)
SPEC_LIST *spec_list;
SM_BUF *output;
{
    long i;
    int num_runs;
    SM_BUF *out_p;
    char temp_buf[PATH_LEN];
    OLDSM_EVAL *eval;
    int inst;
    int num_queries;
    int missing_query_flag;

    if (spec_list == NULL || spec_list->num_spec <= 0)
        return;

    if (output == NULL) {
        out_p = &internal_output;
        out_p->end = 0;
    }
    else
        out_p = output;

    /* Reserve space for each evaluated run */
    if (NULL == (eval = (OLDSM_EVAL *) 
                 malloc ((unsigned) spec_list->num_spec * sizeof (OLDSM_EVAL))))
        return;

    if (UNDEF == add_buf_string ("            Top ranked evaluation\n\n",
                                 out_p))
        return;

    /* Get each evaluated run in turn */
    num_runs = 0;
    num_queries = 0;
    for (i = 0; i < spec_list->num_spec; i++) {
        if (UNDEF == lookup_spec (spec_list->spec[i],
                                  &spec_args[0],
                                  num_spec_args))
            return;
        if (UNDEF == (inst = init_tr_oeval (spec_list->spec[i],
                                           (char *) NULL)) ||
            UNDEF == tr_oeval ((char *) NULL, &eval[num_runs], inst) ||
            UNDEF == close_tr_oeval (inst)) {
            (void) sprintf(temp_buf,
                           "***ERROR*** Run %s cannot be evaluated. Ignored\n",
                           VALID_FILE (run_name) ? run_name : "Unknown");
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
            continue;
        }
        /* Keep track of the maximum number of queries retrieved (or wanted)
           in all runs */
        if (eval[num_runs].qid > num_queries)
            num_queries = eval[num_runs].qid;
        if (run_num_queries > num_queries)
            num_queries = run_num_queries;

        num_runs++;
        (void) sprintf (temp_buf, "%d. %s\n", num_runs,
                        VALID_FILE (run_name) ? run_name : tr_file);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }

    /* Determine if any queries were missing for any runs */
    missing_query_flag = 0;
    for (i = 0; i < num_runs; i++) {
        if (eval[i].qid != num_queries)
            missing_query_flag = 1;        
    }

    if (missing_query_flag) {
        if (UNDEF == add_buf_string ("\n***WARNING*** Number of queries per run is inconsistent.\n    First set of results will give actual figures.\n    Second set of results will give average figures normalized by\n    assuming all missing queries have evaluation figures of 0.0\n",
                                     out_p))
            return;
    }
    print_eval_list (eval, num_runs, out_p);

    if (missing_query_flag) {
        /* Have to redo the evaluation since queries are missing. */
        redo_eval (eval, num_runs, num_queries);
        if (UNDEF == add_buf_string ("\n***Second set of normalized evaluation figures***\n",
                                     out_p))
            return;
        print_eval_list (eval, num_runs, out_p);
    }

    if (output == NULL) {
        (void) fwrite (out_p->buf, 1, out_p->end, stdout);
        out_p->end = 0;
    }
}

void
print_rr_oeval (spec_list, output)
SPEC_LIST *spec_list;
SM_BUF *output;
{
    long i;
    int num_runs;
    SM_BUF *out_p;
    char temp_buf[PATH_LEN];
    OLDSM_EVAL *eval;
    int inst;

    if (spec_list == NULL || spec_list->num_spec <= 0)
        return;
    
    if (output == NULL) {
        out_p = &internal_output;
        out_p->end = 0;
    }
    else
        out_p = output;
    
    /* Reserve space for each evaluated run */
    if (NULL == (eval = (OLDSM_EVAL *) 
                 malloc ((unsigned) spec_list->num_spec * sizeof (OLDSM_EVAL))))
        return;
    
    if (UNDEF == add_buf_string("           Relevant ranked evaluation\n\n",
                                out_p))
        return;

    /* Get each evaluated run in turn */
    num_runs = 0;
    for (i = 0; i < spec_list->num_spec; i++) {
        if (UNDEF == lookup_spec (spec_list->spec[i],
                                  &spec_args[0],
                                  num_spec_args))
            return;
        if (UNDEF == (inst = init_rr_oeval (spec_list->spec[i],
                                           (char *) NULL)) ||
            UNDEF == rr_oeval ((char *) NULL, &eval[num_runs], inst) ||
            UNDEF == close_rr_oeval (inst)) {
            (void) sprintf(temp_buf,
                           "***ERROR*** Run %s cannot be evaluated. Ignored\n",
                           VALID_FILE (run_name) ? run_name : "Unknown");
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
            continue;
        }
        num_runs++;
        (void) sprintf (temp_buf, "%d. %s\n", num_runs,
                        VALID_FILE (run_name) ? run_name : rr_file);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    if (num_runs == 0) {
        if (UNDEF == add_buf_string ("\n***ERROR*** No valid runs included\n",
                                     out_p))
            return;
        return;
    }
    print_eval_list (eval, num_runs, out_p);

    if (output == NULL) {
        (void) fwrite (out_p->buf, 1, out_p->end, stdout);
        out_p->end = 0;
    }
}


static void
print_eval_list (eval, num_runs, out_p)
OLDSM_EVAL *eval;
int num_runs;
SM_BUF *out_p;
{
    long i,j;
    char temp_buf[PATH_LEN];

    if (UNDEF == add_buf_string
        ("\nRun number:      ", out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "   %2d    ", i+1);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    /* Print total numbers retrieved/rel for all runs */
    if (UNDEF == add_buf_string("\nNum_queries:  ", out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "    %5ld", eval[i].qid);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    if (UNDEF == add_buf_string("\nTotal number of documents over all queries",
                                out_p))
        return;
    if (UNDEF == add_buf_string("\n    Retrieved:", out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "    %5ld", eval[i].num_ret);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    if (UNDEF == add_buf_string("\n    Relevant: ", out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "    %5ld", eval[i].num_rel);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    if (UNDEF == add_buf_string("\n    Rel_ret:  ", out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "    %5ld", eval[i].num_rel_ret);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    if (UNDEF == add_buf_string("\n    Trunc_ret:", out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "    %5ld", eval[i].num_trunc_ret);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }

    /* Print recall precision figures at ONUM_RP_PTS recall levels */
    if (UNDEF == add_buf_string
        ("\nRecall - Precision Averages:", out_p))
        return;
    for (j = 0; j < ONUM_RP_PTS; j++) {
        (void) sprintf (temp_buf, "\n    at %4.2f     ",
                        (float) j / (ONUM_RP_PTS - 1));
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
        for (i = 0; i < num_runs; i++) {
            (void) sprintf (temp_buf, "  %6.4f ", eval[i].recall_precis[j]);
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
        }
    }

    /* Print 3 point average recall precision and percentage improvement */
    if (UNDEF == add_buf_string
        ("\nAverage precision for 3 intermediate points (0.25, 0.50, 0.75)\n    3-pt Avg:   ",
         out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "  %6.4f ", eval[i].av_recall_precis);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    if (num_runs > 1) {
        (void) sprintf (temp_buf, "\n    %% Change:           ");
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
        for (i = 1; i < num_runs; i++) {
            (void) sprintf (temp_buf, "  %6.1f ",
                            ((eval[i].av_recall_precis /
                              eval[0].av_recall_precis)
                             - 1.0) * 100.0);
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
        }
    }

    (void) sprintf (temp_buf, "\nRecall:\n    Exact:     ");
    if (UNDEF == add_buf_string (temp_buf, out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "   %6.4f", eval[i].exact_recall);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    for (j = 0; j < NUM_CUTOFF; j++) {
        (void) sprintf (temp_buf, "\n    at %2d docs:", cutoff[j]);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
        for (i = 0; i < num_runs; i++) {
            (void) sprintf (temp_buf, "   %6.4f", eval[i].recall_cut[j]);
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
        }
    }
    (void) sprintf (temp_buf, "\nPrecision:\n    Exact:     ");
    if (UNDEF == add_buf_string (temp_buf, out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "   %6.4f", eval[i].exact_precis);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    for (j = 0; j < NUM_CUTOFF; j++) {
        (void) sprintf (temp_buf, "\n    At %2d docs:", cutoff[j]);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
        for (i = 0; i < num_runs; i++) {
            (void) sprintf (temp_buf, "   %6.4f", eval[i].precis_cut[j]);
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
        }
    }
    (void) sprintf (temp_buf, "\nTruncated Precision:\n    Exact:     ");
    if (UNDEF == add_buf_string (temp_buf, out_p))
        return;
    for (i = 0; i < num_runs; i++) {
        (void) sprintf (temp_buf, "   %6.4f", eval[i].exact_trunc_precis);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
    }
    for (j = 0; j < NUM_CUTOFF; j++) {
        (void) sprintf (temp_buf, "\n    At %2d docs:", cutoff[j]);
        if (UNDEF == add_buf_string (temp_buf, out_p))
            return;
        for (i = 0; i < num_runs; i++) {
            (void) sprintf (temp_buf, "   %6.4f", eval[i].trunc_cut[j]);
            if (UNDEF == add_buf_string (temp_buf, out_p))
                return;
        }
    }
    if (UNDEF == add_buf_string ("\n", out_p))
        return;
}


static void 
redo_eval (eval, num_runs, num_queries)
OLDSM_EVAL *eval;
int num_runs;
int num_queries;
{
    long i,j;
    double factor;

    for (i = 0; i < num_runs; i++) {
        if (eval[i].qid == num_queries)
            continue;
        factor = (double) eval[i].qid / num_queries;
        eval[i].qid = num_queries;

        for (j = 0; j < ONUM_RP_PTS; j++) {
            eval[i].recall_precis[j] *= factor;
        }
        eval[i].av_recall_precis *= factor;
        eval[i].exact_recall *= factor;
        eval[i].exact_precis *= factor;
        eval[i].exact_trunc_precis *= factor;
        for (j = 0; j < NUM_CUTOFF; j++) {
            eval[i].recall_cut[j] *= factor;
            eval[i].precis_cut[j] *= factor;
            eval[i].trunc_cut[j] *= factor;
        }
    }
}
