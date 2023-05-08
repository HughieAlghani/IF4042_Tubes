#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfeedback/form_query.c,v 11.0 1992/07/21 18:20:41 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "vector.h"
#include "feedback.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "retrieve.h"


static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("feedback.form_query.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

int
init_form_query (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_form_query");

    PRINT_TRACE (2, print_string, "Trace: leaving init_form_query");
    return (1);
}

int
form_query (fdbk_info, new_query, inst)
FEEDBACK_INFO *fdbk_info;
QUERY_VECTOR *new_query;
int inst;
{
    long i;
    long max_ctype;
    CON_WT *conwt_ptr;
    VEC *vec;
    CON_WT *con_wt;
    long num_conwt;
    long *ctype_len;


    PRINT_TRACE (2, print_string, "Trace: entering form_query");

    num_conwt = 0; max_ctype = -1;
    for (i = 0; i < fdbk_info->num_occ; i++) {
        if (fdbk_info->occ[i].ctype > max_ctype)
            max_ctype = fdbk_info->occ[i].ctype;
        if (fdbk_info->occ[i].weight != 0.0)
            num_conwt++;
    }

    if (NULL == (con_wt = (CON_WT *) malloc ((unsigned) num_conwt
                                                 * sizeof (CON_WT))) ||
        NULL == (ctype_len = (long *) malloc ((unsigned) (1 + max_ctype)
                                               * sizeof (long))) ||
        NULL == (vec = (VEC *) malloc (sizeof (VEC)))) {
        set_error (errno, "", "form_query");
        return (UNDEF);
    }
    for (i = 0; i <= max_ctype; i++) {
        ctype_len[i] = 0;
    }

    conwt_ptr = con_wt;
    for (i = 0; i < fdbk_info->num_occ; i++) {
        if (fdbk_info->occ[i].weight == 0.0)
            continue;

        conwt_ptr->con = fdbk_info->occ[i].con;
        conwt_ptr->wt = fdbk_info->occ[i].weight;
        ctype_len[fdbk_info->occ[i].ctype]++;
        conwt_ptr++;
    }

    vec->ctype_len = ctype_len;
    vec->con_wtp = con_wt;
    vec->num_ctype = max_ctype + 1;
    vec->num_conwt = num_conwt;
    vec->id_num = fdbk_info->orig_query->id_num;

    /* Cosine normalize the entire query */
    if (UNDEF == normwt_cos (vec, (char *) NULL, 0))
        return (UNDEF);

    new_query->qid = vec->id_num;
    new_query->vector = (char *) vec;

    
    PRINT_TRACE (4, print_vector, vec);
    PRINT_TRACE (2, print_string, "Trace: leaving form_query");
    return (1);
}

int
close_form_query (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_form_query");

    PRINT_TRACE (2, print_string, "Trace: leaving close_form_query");
    return (1);
}
