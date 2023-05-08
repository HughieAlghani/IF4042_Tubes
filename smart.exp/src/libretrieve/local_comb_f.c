#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/local_comb_f.c,v 11.0 1992/07/21 18:23:54 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Assign sim to doc by using global sim + constant if doc has local_sim > 0 
 *1 retrieve.local_comb.fill
 *2 local_comb_fill (local_top_results, results, inst)
 *3 TOP_RESULT *local_top_results;
 *3 RESULT *results;
 *3 int inst;
 *4 init_local_comb_fill (spec, unused)
 *5   "retrieve.local_comb.trace"
 *4 close_local_comb_fill(inst)
 *7 local_top_results[i] gives a local similarity (eg computed by
 *7 local_thresh) corresponding to the global similarity in
 *7 results->top_results[i].  
 *7 Want the final ranking to be decreasing global sim among those docs
 *7 with non-zero local sim, and then decreasing global sim among those docs
 *7 with zero local sim.  Do this by adding floor (10 + max_global_sim) to
 *7 the sim of each doc with non-zero local sim, and leaving other
 *7 global sims unchanged.
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

int
init_local_comb_fill (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_local_comb_fill");

    PRINT_TRACE (2, print_string, "Trace: leaving init_local_comb_fill");

    return (0);
}

int
local_comb_fill (local_top_results, results, inst)
TOP_RESULT *local_top_results;
RESULT *results;
int inst;
{
    long i;
    float increment;
    RESULT temp_results;

    PRINT_TRACE (2, print_string, "Trace: entering local_comb_fill");
    PRINT_TRACE (6, print_top_results, results);
    /* Hack to trace local_top_results */
    temp_results.top_results = local_top_results;
    temp_results.num_top_results = results->num_top_results;
    PRINT_TRACE (6, print_top_results, &temp_results);

    increment = (float) floor ((double) 10.0 + results->top_results[0].sim);
    for (i = 0; i < results->num_top_results; i++) {
        if (local_top_results[i].sim > 0.0)
            results->top_results[i].sim += increment;
    }

    qsort ((char *) results->top_results,
           (int) results->num_top_results,
           sizeof (TOP_RESULT),
           comp_sim_did);

    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving local_comb_fill");

    return (1);
}

int
close_local_comb_fill(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_local_comb_fill");

    PRINT_TRACE (2, print_string, "Trace: leaving close_local_comb_fill");

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

