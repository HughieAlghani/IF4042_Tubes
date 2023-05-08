#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/rank_parts_s.c,v 11.0 1992/07/21 18:23:52 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Assign sim to doc based on sum of sims of already computed doc parts
 *1 retrieve.rank_parts.sum
 *2 rank_parts_sum (parts_partial, results, inst)
 *3 struct {
 *3         long max_doc;
 *3         long max_parts;
 *3         float *sum_results;
 *3         float *max_results;
 *3         int *num_parts;
 *3 } *parts_partial;
 *3 RESULT *results;
 *3 int inst;
 *4 init_rank_parts_sum (spec, unused)
 *5   "retrieve.rank_tr"
 *5   "retrieve.rank_parts.trace"
 *4 close_rank_parts_sum(inst)
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

static PROC_TAB *rank_tr;

static SPEC_PARAM spec_args[] = {
    "retrieve.rank_tr",          getspec_func, (char *) &rank_tr,
    TRACE_PARAM ("retrieve.rank_parts.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


static long rank_tr_inst;

int
init_rank_parts_sum (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_rank_parts_sum");

    /* Call all initialization procedures */
    if (UNDEF == (rank_tr_inst = rank_tr->init_proc (spec, NULL))) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_rank_parts_sum");

    return (0);
}

int
rank_parts_sum (parts_partial, results, inst)
PARTS_RESULTS *parts_partial;
RESULT *results;
int inst;
{
    long i;
    TOP_RESULT new_top;
    float *top_thresh;

    PRINT_TRACE (2, print_string, "Trace: entering rank_parts_sum");

    results->full_results = parts_partial->sum_results;

    if (results->num_top_results > 0) {
        top_thresh = &(results->top_results[results->num_top_results-1].sim);
        for (i = 0; i < parts_partial->max_doc; i++) {
            if (parts_partial->sum_results[i] >= *top_thresh) {
                new_top.sim = parts_partial->sum_results[i];
                new_top.did = i;
                if (UNDEF == rank_tr->proc (&new_top,
                                            results,
                                            rank_tr_inst))
                    return (UNDEF);
            }
        }
    }

    PRINT_TRACE (2, print_string, "Trace: leaving rank_parts_sum");

    return (1);
}

int
close_rank_parts_sum(inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_rank_parts_sum");

    if (UNDEF == rank_tr->close_proc(rank_tr_inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_rank_parts_sum");

    return (0);
}
