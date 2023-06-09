#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/sim_inner.c,v 11.0 1992/07/21 18:23:57 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Perform innerproduct similarity function between two vectors
 *1 retrieve.ctype_vec.inner
 *2 sim_inner (vec_pair, sim_ptr, inst)
 *3   VEC_PAIR *vec_pair;
 *3   float *sim_ptr;
 *3   int inst;
 *4 init_sim_inner (spec, param_prefix)
 *5   "retrieve.ctype_vec.trace"
 *5   "ctype.*.sim_ctype_weight"
 *4 close_sim_inner (inst)

 *7 Calculate the inner product of the two vectors given by vec_pair.
 *7 The vectors are most often a single ctype, and their concepts are
 *7 assumed to be sorted in increasing concept number.  The final sim is
 *7 multiplied by the ctype weight  specified by the parameter whose 
 *7 name is the concatenation of param_prefix and "sim_ctype_weight".
 *7 Normally this parameter name will be "ctype.N.sim_ctype_weight", 
 *7 where N is the ctype for this query.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "retrieve.h"
#include "vector.h"
#include "inst.h"

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("retrieve.ctype_vec.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static char *prefix;
static float ctype_weight;
static SPEC_PARAM_PREFIX pspec_args[] = {
    &prefix,  "sim_ctype_weight", getspec_float, (char *) &ctype_weight,
    };
static int num_pspec_args = sizeof (pspec_args) / sizeof (pspec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    float ctype_weight;                /* Weight for this particular ctype */
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

int
init_sim_inner (spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
    STATIC_INFO *ip;
    int new_inst;

    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);

    ip = &info[new_inst];

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);
    prefix = param_prefix;
    if (UNDEF == lookup_spec_prefix (spec, &pspec_args[0], num_pspec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_inner");

    ip->ctype_weight = ctype_weight;

    ip->valid_info++;

    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_inner");
    return (new_inst);
}

int
sim_inner (vec_pair, sim_ptr, inst)
VEC_PAIR *vec_pair;
float *sim_ptr;
int inst;
{
    register CON_WT *vec1_ptr, *vec1_end, *vec2_ptr, *vec2_end;
    double sim = 0.0;

    PRINT_TRACE (2, print_string, "Trace: entering sim_inner");
    PRINT_TRACE (6, print_vector, vec_pair->vec1);
    PRINT_TRACE (6, print_vector, vec_pair->vec2);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "sim_inner");
        return (UNDEF);
    }

    vec1_ptr = vec_pair->vec1->con_wtp;
    vec1_end = &vec_pair->vec1->con_wtp[vec_pair->vec1->num_conwt];
    vec2_ptr = vec_pair->vec2->con_wtp;
    vec2_end = &vec_pair->vec2->con_wtp[vec_pair->vec2->num_conwt];

    if (vec1_ptr >= vec1_end ||
        vec2_ptr >= vec2_end ||
        info[inst].ctype_weight == 0.0) {
        *sim_ptr = 0.0;
    }
    else {
        while (1) {
            if (vec1_ptr->con < vec2_ptr->con) {
                if (++vec1_ptr >= vec1_end)
                    break;
            }
            else if (vec1_ptr->con > vec2_ptr->con){
                if (++vec2_ptr >= vec2_end)
                    break;
            }
            else {          /* vec1_ptr->con == vec2_ptr->con */
                sim += vec1_ptr->wt * vec2_ptr->wt;
                if (++vec1_ptr >= vec1_end || ++vec2_ptr >= vec2_end)
                    break;
            }
        }
        *sim_ptr = info[inst].ctype_weight * sim;
    }
    
    PRINT_TRACE (4, print_float, sim_ptr);
    PRINT_TRACE (2, print_string, "Trace: leaving sim_inner");
    return (1);
}


int
close_sim_inner (inst)
int inst;
{
    PRINT_TRACE (2, print_string,
                 "Trace: entering/leaving close_sim_inner");
    return (0);
}
