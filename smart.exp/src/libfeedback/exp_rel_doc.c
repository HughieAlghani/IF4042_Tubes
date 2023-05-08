#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libfeedback/exp_rel_doc.c,v 11.0 1992/07/21 18:20:39 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "tr_vec.h"
#include "vector.h"
#include "feedback.h"
#include "spec.h"
#include "trace.h"

/* Expand query by adding all terms occurring in rel docs */

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("feedback.expand.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


static int add_doc();
static int compare_conq();
static void collapse();

static int did_vec_inst;

int
init_exp_rel_doc (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_exp_rel_doc");

    if (UNDEF == (did_vec_inst = init_did_vec (spec, (char *) NULL)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_exp_rel_doc");
    return (1);
}

int
exp_rel_doc (unused, fdbk_info, inst)
char *unused;
FEEDBACK_INFO *fdbk_info;
int inst;
{
    long i;
    VEC dvec;

    PRINT_TRACE (2, print_string, "Trace: entering exp_rel_doc");

    fdbk_info->num_occ = 0;
    fdbk_info->num_rel = 0;

    /* add the original query to list */
    if (UNDEF == add_doc (fdbk_info, fdbk_info->orig_query, 1))
        return (UNDEF);

    /* Add each rel retrieved doc to list */
    for (i = 0; i < fdbk_info->tr->num_tr; i++) {
        if (fdbk_info->tr->tr[i].rel == 1) {
            if (1 != did_vec (&(fdbk_info->tr->tr[i].did),
                              &dvec,
                              did_vec_inst))
                return (UNDEF);
            if (UNDEF == add_doc (fdbk_info, &dvec, 0))
                return (UNDEF);
            fdbk_info->num_rel++;
        }
    }
    /* Remove duplicate entries by sorting and collapsing. reset rel_ret */
    /* field and rel_weight field */
    if (fdbk_info->num_rel > 0 ) {
        qsort ((char *) fdbk_info->occ,
               (int) fdbk_info->num_occ, sizeof (OCC), compare_conq);
        collapse (fdbk_info);
    }

    PRINT_TRACE (4, print_fdbk_info, fdbk_info);
    PRINT_TRACE (2, print_string, "Trace: leaving exp_rel_doc");
    return (1);
}


int
close_exp_rel_doc (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_exp_rel_doc");

    if (UNDEF == close_did_vec (did_vec_inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_exp_rel_doc");
    return (0);
}



static int
add_doc (fdbk_info, dvec, orig_query_flag)
FEEDBACK_INFO *fdbk_info;
VEC *dvec;
int orig_query_flag;
{
    OCC *new_occ;
    CON_WT *conwt;
    long ctype;
    long i;

    if (fdbk_info->max_occ < fdbk_info->num_occ + dvec->num_conwt) {
        /* Must reserve more space for occ info */
        if (NULL == (new_occ = (OCC *)
                     malloc ((unsigned) (fdbk_info->num_occ +
                                         dvec->num_conwt) * sizeof (OCC))))
            return (UNDEF);
        if (fdbk_info->num_occ > 0)
            bcopy ((char *) fdbk_info->occ, 
                   (char *) new_occ,
                   (int) fdbk_info->num_occ * sizeof (OCC));
        if (fdbk_info->max_occ > 0)
            free ((char *) fdbk_info->occ);
        fdbk_info->occ = new_occ;
        fdbk_info->max_occ = fdbk_info->num_occ + dvec->num_conwt;
    }


    /* Zero out fields of new occ info (assumes all zero bits means float 0) */
    bzero ((char *) &fdbk_info->occ[fdbk_info->num_occ],
           (int) dvec->num_conwt * sizeof (OCC));
    
    new_occ = &fdbk_info->occ[fdbk_info->num_occ];
    conwt = dvec->con_wtp;
    for (ctype = 0; ctype < dvec->num_ctype; ctype++) {
        for (i = 0; i < dvec->ctype_len[ctype]; i++) {
            new_occ->con = conwt->con;
            new_occ->ctype = ctype;
            if (orig_query_flag) {
                new_occ->orig_weight = conwt->wt;
                new_occ->query = 1;
            }
            else {
                new_occ->rel_weight = conwt->wt;
            }
            conwt++;
            new_occ++;
        }
    }
    fdbk_info->num_occ += dvec->num_conwt;

    return (1);
}

/* Sort by ctype.  Within ctype, sort by con. Within con, put entry for
   original query first */
static int
compare_conq (ptr1, ptr2)
OCC *ptr1;
OCC *ptr2;
{
    if (ptr1->ctype < ptr2->ctype)
        return (-1);
    if (ptr1->ctype > ptr2->ctype)
        return (1);
    if (ptr1->con < ptr2->con)
        return (-1);
    if (ptr1->con > ptr2->con)
        return (1);
    if (ptr1->query)
        return (-1);
    if (ptr2->query)
        return (1);
    return (0);
}


/* Collapse multiple entries for term onto one entry, keeping track of rel
   occurrences and sum of weights */
static void
collapse (fdbk_info)
FEEDBACK_INFO *fdbk_info;
{
    OCC *last;           /* pointer to last valid entry */
    OCC *current;        /* pointer to entry to be decided if kept */

    last = fdbk_info->occ;
    last->rel_ret = (last->query) ? 0 : 1;

    for (current = fdbk_info->occ + 1;
         current < &fdbk_info->occ[fdbk_info->num_occ];
         current++) {
        if (current->con == last->con &&
            current->ctype == last->ctype) {
                last->rel_weight += current->rel_weight;
            last->rel_ret++;
        }
        else {
            last++;
            last->con = current->con;
            last->ctype = current->ctype;
            last->orig_weight = current->orig_weight;
            last->rel_weight = current->rel_weight;
            last->query = current->query;
            last->rel_ret = (current->query) ? 0 : 1;
        }
    }

    fdbk_info->num_occ = (last - fdbk_info->occ) + 1;
}


