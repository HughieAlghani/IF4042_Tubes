#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/vecs_vecs.c,v 11.0 1992/07/21 18:24:02 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Compute pairwise similarities between two lists of vectors
 *1 retrieve.vecs_vecs.vecs_vecs
 *2 vecs_vecs (vec_list_pair, results , inst)
 *3   VEC_LIST_PAIR *vec_list_pair;
 *3   ALT_RESULTS *results;
 *3   int inst;
 *4 init_vecs_vecs (spec, unused)
 *5   "retrieve.vecs_vecs.vec_vec"
 *5   "retrieve.vecs_vecs.trace"
 *4 close_vecs_vecs (inst)
 *7 Each vector in vec_list_pair->vec_list1 is compared against every
 *7 vector in vec_list_pair->vec_list2.  All non-zero similarities are
 *7 returned in results.  The comparison between vectors is done by the
 *7 procedure given by vec_vec.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "vector.h"
#include "retrieve.h"

/* Sequentially compare a group of vecs against another group of vecs,
   reporting all non_zero similarities */

static PROC_INST vec_vec;       /* Procedure to calc sim for individual
                                   vector against individual vector */

static SPEC_PARAM spec_args[] = {
    "retrieve.vecs_vecs.vec_vec", getspec_func, (char *) &vec_vec.ptab,
    TRACE_PARAM ("retrieve.vecs_vecs.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static long num_result_space;
static RESULT_TUPLE *result_buf;

static int num_inst = 0;

int
init_vecs_vecs (spec, unused)
SPEC *spec;
char *unused;
{
    if (num_inst++) {
        PRINT_TRACE (2, print_string,
                     "Trace: entering/leaving init_vecs_vecs");
        return (0);
    }

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_vecs_vecs");

    /* Call all initialization procedures */
    if (UNDEF == (vec_vec.inst =vec_vec.ptab->init_proc (spec, NULL))) {
        return (UNDEF);
    }

    num_result_space = 0;

    PRINT_TRACE (2, print_string, "Trace: leaving init_vecs_vecs");
    return (0);
}

int
vecs_vecs (vec_list_pair, results , inst)
VEC_LIST_PAIR *vec_list_pair;
ALT_RESULTS *results;
int inst;
{
    float sim;
    VEC *vec1, *vec2;
    VEC_PAIR vec_pair;

    /* Reserve space for results */
    if (vec_list_pair->vec_list1->num_vec *
        vec_list_pair->vec_list2->num_vec > num_result_space) {
        if (num_result_space != 0)
            free ((char *) result_buf);
        num_result_space = vec_list_pair->vec_list1->num_vec *
            vec_list_pair->vec_list2->num_vec;
        if (NULL == (result_buf = (RESULT_TUPLE *)
                     malloc ((unsigned) num_result_space * sizeof (RESULT_TUPLE))))
            return (UNDEF);
    }

    results->results = result_buf;
    results->num_results = 0;

    /* Compare list1 against list2 using sim measure add */
    for (vec1 = vec_list_pair->vec_list1->vec;
         vec1 < &vec_list_pair->vec_list1->vec[vec_list_pair->vec_list1->num_vec];
         vec1++) {
        if (vec1->num_conwt == 0)
            continue;
        for (vec2 = vec_list_pair->vec_list2->vec;
             vec2 < &vec_list_pair->vec_list2->vec[vec_list_pair->vec_list2->num_vec];
             vec2++) {
            if (vec2->num_conwt == 0)
                continue;
            vec_pair.vec1 = vec1;
            vec_pair.vec2 = vec2;
            if (UNDEF == vec_vec.ptab->proc (&vec_pair, &sim, vec_vec.inst))
                return (UNDEF);
            if (sim > 0.0) {
                result_buf[results->num_results].qid = vec1->id_num;
                result_buf[results->num_results].did = vec2->id_num;
                result_buf[results->num_results].sim = sim;
                results->num_results++;
            }
        }
    }

    return (1);
}


int
close_vecs_vecs (inst)
int inst;
{
    if (--num_inst) {
        PRINT_TRACE (2, print_string,
                     "Trace: entering/leaving close_vecs_vecs");
        return (0);
    }

    PRINT_TRACE (2, print_string, "Trace: entering close_vecs_vecs");

    if (num_result_space > 0)
        (void) free ((char *) result_buf);
    num_result_space = 0;
    if (UNDEF == vec_vec.ptab->close_proc (vec_vec.inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_vecs_vecs");
    return (0);
}

