#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/local_comb_v.c,v 11.0 1992/07/21 18:24:03 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Assign sim to doc by using global sim if doc has high local_sim
 *1 retrieve.local_comb.var
 *2 local_comb_var (local_top_results, results, inst)
 *3 TOP_RESULT *local_top_results;
 *3 RESULT *results;
 *3 int inst;
 *4 init_local_comb_var (spec, unused)
 *5   "retrieve.local_comb.trace"
 *4 close_local_comb_var(inst)
 *7 local_top_results[i] gives a local similarity (eg computed by
 *7 local_thresh) corresponding to the global similarity in
 *7 results->top_results[i].
 *7 Find the median value of the local sims (half the docs have local
 *7 sim above this value).  Assign final global_sim[i] to be 0.0 if
 *7 local_sim[i] is below this value else leave it unchanged.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "io.h"
#include "proc.h"
#include "retrieve.h"

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("retrieve.local_comb.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


static int comp_sim_did ();
static float get_median();

int
init_local_comb_var (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_local_comb_var");

    PRINT_TRACE (2, print_string, "Trace: leaving init_local_comb_var");

    return (0);
}

int
local_comb_var (local_top_results, results, inst)
TOP_RESULT *local_top_results;
RESULT *results;
int inst;
{
    long i;
    float median;
    RESULT temp_results;

    PRINT_TRACE (2, print_string, "Trace: entering local_comb_var");
    PRINT_TRACE (6, print_top_results, results);
    /* Hack to trace local_top_results */
    temp_results.top_results = local_top_results;
    temp_results.num_top_results = results->num_top_results;
    PRINT_TRACE (6, print_top_results, &temp_results);

    median = get_median (local_top_results, results->num_top_results);

    for (i = 0; i < results->num_top_results; i++) {
        if (local_top_results[i].sim < median)
            results->top_results[i].sim = 0.0;
    }

    qsort ((char *) results->top_results,
           (int) results->num_top_results,
           sizeof (TOP_RESULT),
           comp_sim_did);

    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving local_comb_var");

    return (1);
}

int
close_local_comb_var(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_local_comb_var");

    PRINT_TRACE (2, print_string, "Trace: leaving close_local_comb_var");

    return (0);
}

static int 
comp_sim_did (top1, top2)
TOP_RESULT *top1, *top2;
{
    if (top1->sim > top2->sim)
        return (-1);
    if (top1->sim < top2->sim)
        return (1);
    if (top1->did > top2->did)
        return (-1);
    if (top1->did < top2->did)
        return (1);
    return (0);
}


static float
get_median (local_top_results, num_top_results)
TOP_RESULT *local_top_results;
long num_top_results;
{
    TOP_RESULT *temp_results;
    float median;

    if (NULL == (temp_results = (TOP_RESULT *)
                 malloc ((unsigned) num_top_results * sizeof (TOP_RESULT))))
        return (0.0);

    (void) bcopy ((char *) local_top_results,
                  (char *) temp_results,
                  (int) num_top_results * sizeof (TOP_RESULT));

    qsort ((char *) temp_results,
           (int) num_top_results,
           sizeof (TOP_RESULT),
           comp_sim_did);

    median = temp_results[num_top_results / 2].sim;

    (void) free ((char *) temp_results);

    return (median);
}

