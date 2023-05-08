/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Perform innerproduct similarity between two vectors, require word matches
 *1 local.ctype_vec.inner_match
 *2 sim_inner_match (vec_pair, sim_ptr, inst)
 *3   VEC_PAIR *vec_pair;
 *3   float *sim_ptr;
 *3   int inst;
 *4 init_sim_inner_match (spec, param_prefix)
 *5   "retrieve.ctype_vec.match"
 *5   "retrieve.ctype_vec.trace"
 *5   "ctype.*.sim_ctype_weight"
 *4 close_sim_inner_match (inst)

 *7 Calculate the inner product of the two vectors given by vec_pair.
 *7 However, if there are not at least `match' terms in common, return
 *7 a similarity of 0.  Note that if match is zero, this function has
 *7 exactly the same effect as retrieve.ctype_vec.inner.
 *7
 *7 The vectors are most often a single ctype, and their concepts are
 *7 assumed to be sorted in increasing concept number.  The final sim is
 *7 multiplied by the ctype weight  specified by the parameter whose 
 *7 name is the concatenation of param_prefix and "sim_ctype_weight".
 *7 Normally this parameter name will be "ctype.N.sim_ctype_weight", 
 *7 where N is the ctype for this query.

 *8 This should really be implemented at the level above, since this
 *8 requires a certain number within the SAME ctype.  What about
 *8 across ctypes?

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

static long match_thresh;

static SPEC_PARAM spec_args[] = {
    "retrieve.ctype_vec.match",      getspec_long, (char *)&match_thresh,
    TRACE_PARAM ("retrieve.ctype_vec.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    float ctype_weight;                /* Weight for this particular ctype */
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

int
init_sim_inner_match (spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
    STATIC_INFO *ip;
    SPEC_PARAM ctype_param;
    char param[PATH_LEN];
    int new_inst;

    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);

    ip = &info[new_inst];

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_inner_match");

    /* Lookup the ctype_weight wanted for this application (normally
       param_prefix will be "ctype.<digit>.") */
    if (NULL != param_prefix) 
        (void) strcpy (param, param_prefix);
    else
        param[0] = '\0';
    (void) strcat (param, "sim_ctype_weight");
    ctype_param.param = param;
    ctype_param.convert = getspec_float;
    ctype_param.result = (char *) &ip->ctype_weight;
    if (UNDEF == lookup_spec (spec, &ctype_param, 1))
        return (UNDEF);

    ip->valid_info++;

    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_inner_match");
    return (new_inst);
}

int
sim_inner_match (vec_pair, sim_ptr, inst)
VEC_PAIR *vec_pair;
float *sim_ptr;
int inst;
{
    register CON_WT *vec1_ptr, *vec1_end, *vec2_ptr, *vec2_end;
    double sim = 0.0;
    int match_count = 0;

    PRINT_TRACE (2, print_string, "Trace: entering sim_inner_match");
    PRINT_TRACE (6, print_vector, vec_pair->vec1);
    PRINT_TRACE (6, print_vector, vec_pair->vec2);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "sim_inner_match");
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
		match_count++;
                if (++vec1_ptr >= vec1_end || ++vec2_ptr >= vec2_end)
                    break;
            }
        }
	if ( match_count >= match_thresh )
	    *sim_ptr = info[inst].ctype_weight * sim;
	else
	    *sim_ptr = 0.0;
    }
    
    PRINT_TRACE (4, print_float, sim_ptr);
    PRINT_TRACE (2, print_string, "Trace: leaving sim_inner_match");
    return (1);
}


int
close_sim_inner_match (inst)
int inst;
{
    PRINT_TRACE (2, print_string,
                 "Trace: entering/leaving close_sim_inner_match");
    return (0);
}
