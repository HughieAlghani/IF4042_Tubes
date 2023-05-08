#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfeedback/wt_fdbk.c,v 11.0 1992/07/21 18:20:44 chrisb Exp $";
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
#include "trace.h"
#include "feedback.h"

/*
 * compute the feedback weight for an individual query term i according
 * to the formula (Roccio feedback):
 *                     sum(term weight in rel)        sum (term weight in nrel)
 * Q' = A * Qold + B * ------------------------ - C * -------------------------
 *                        num_rel_doc                     num_nrel_doc
 *
 *  where num_rel_doc is the total number of relevant docs 
 *  and A,B,C are constants (feedback.alpha, feedback.beta, feedback.gamma)
 */

static float alpha;
static float beta;
static float gamma;

static SPEC_PARAM spec_args[] = {
    "feedback.alpha",         getspec_float, (char *) &alpha,
    "feedback.beta",          getspec_float, (char *) &beta,
    "feedback.gamma",         getspec_float, (char *) &gamma,
    TRACE_PARAM ("feedback.weight.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


int
init_wt_fdbk (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_wt_fdbk");
    PRINT_TRACE (6, print_spec, spec);

    PRINT_TRACE (2, print_string, "Trace: leaving init_wt_fdbk");
    return (1);
}

int
wt_fdbk (unused, fdbk_info, inst)
char *unused;
FEEDBACK_INFO *fdbk_info;
int inst;
{
    long i;
    double rel_wt, nrel_wt;

    PRINT_TRACE (2, print_string, "Trace: entering wt_fdbk");

    for (i = 0; i < fdbk_info->num_occ; i++) {
        if (fdbk_info->num_rel > 0) 
            rel_wt = beta * fdbk_info->occ[i].rel_weight /
                fdbk_info->num_rel;
        else
            rel_wt = 0.0;

        if (fdbk_info->tr->num_tr > fdbk_info->num_rel) 
            nrel_wt = gamma * fdbk_info->occ[i].nrel_weight /
                (fdbk_info->tr->num_tr - fdbk_info->num_rel);
        else
            nrel_wt = 0.0;

        fdbk_info->occ[i].weight = alpha * fdbk_info->occ[i].orig_weight + 
                                   rel_wt - nrel_wt;
        if (fdbk_info->occ[i].weight < 0.0)
            fdbk_info->occ[i].weight = 0.0;
    }

    PRINT_TRACE (4, print_fdbk_info, fdbk_info);
    PRINT_TRACE (2, print_string, "Trace: leaving wt_fdbk");

    return (1);
}

int
close_wt_fdbk (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering/leaving close_wt_fdbk");
    return (0);
}

