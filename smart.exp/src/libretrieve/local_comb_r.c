#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/local_comb_r.c,v 11.0 1992/07/21 18:23:56 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Assign sim to doc by using global sim if doc has non-zero local_sim 
 *1 retrieve.local_comb.req
 *2 local_comb_req (local_top_results, results, inst)
 *3 TOP_RESULT *local_top_results;
 *3 RESULT *results;
 *3 int inst;
 *4 init_local_comb_req (spec, unused)
 *5   "retrieve.local_comb.trace"
 *4 close_local_comb_req(inst)
 *7 local_top_results[i] gives a local similarity (eg computed by
 *7 local_thresh) corresponding to the global similarity in
 *7 results->top_results[i].  
 *7 If the local sim is zero, then results->top_results[i] is set
 *7 to 0.0, otherwise results->top_results[i] is left alone.
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
init_local_comb_req (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_local_comb_req");

    PRINT_TRACE (2, print_string, "Trace: leaving init_local_comb_req");

    return (0);
}

int
local_comb_req (local_top_results, results, inst)
TOP_RESULT *local_top_results;
RESULT *results;
int inst;
{
    long i;
    RESULT temp_results;

    PRINT_TRACE (2, print_string, "Trace: entering local_comb_req");
    PRINT_TRACE (6, print_top_results, results);
    /* Hack to trace local_top_results */
    temp_results.top_results = local_top_results;
    temp_results.num_top_results = results->num_top_results;
    PRINT_TRACE (6, print_top_results, &temp_results);

    for (i = 0; i < results->num_top_results; i++) {
        if (local_top_results[i].sim == 0.0)
            results->top_results[i].sim = 0.0;
    }

    qsort ((char *) results->top_results,
           (int) results->num_top_results,
           sizeof (TOP_RESULT),
           comp_sim_did);

    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving local_comb_req");

    return (1);
}

int
close_local_comb_req(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_local_comb_req");

    PRINT_TRACE (2, print_string, "Trace: leaving close_local_comb_req");

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

