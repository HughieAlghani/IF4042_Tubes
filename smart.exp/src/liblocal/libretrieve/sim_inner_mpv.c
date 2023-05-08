/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Perform innerproduct similarity between two part vectors
 *1 local.ctype_vec.inner_partvec_match
 *2 sim_inner_partvec_match (pvec_pair, results, inst)
 *3   PART_VEC_PAIR *pvec_pair;
 *3   RESULT_TUPLE *results;
 *3   int inst;
 *4 init_sim_inner_partvec_match (spec, param_prefix)
 *5   "retrieve.ctype_vec.match"
 *5   "retrieve.ctype_vec.trace"
 *5   "ctype.*.sim_ctype_weight"
 *4 close_sim_inner_partvec_match (inst)

 *7 Each of the part-vectors effectively represents an entire matrix.
 *7 This routine calculates the pairwise inner products of the two
 *7 sets of matrix rows, where the row number is given by the partnum
 *7 in the concept vector.  Similarity for rows i and j is put in
 *7 results[i*maxpartnum + j] where maxpartnum is decided by pvec2 in
 *7 the pair (that is, maxpartnum.1 rows, each maxpartnum.2 wide).
 *7 
 *7 We assume that the passed vectors are of ONE ctype, that the
 *7 results area has been pre-allocated and pre-initialized.

***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "retrieve.h"
#include "part_vector.h"
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

static long num_hits;
static short *hits;	/* for counting number of matching pairs */

#define Malloc(n,type) (type *) malloc( (unsigned) (n)*sizeof(type))

int
init_sim_inner_partvec_match (spec, param_prefix)
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

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_inner_partvec_match");

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

    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_inner_partvec_match");
    return (new_inst);
}

int
sim_inner_partvec_match (pvec_pair, pvec_results, inst)
PART_VEC_PAIR *pvec_pair;
PART_VEC_RESULTS *pvec_results;
int inst;
{
    register PART_CON_WT *vec1_ptr, *vec1_end, *vec2_ptr, *vec2_end;
    register PART_CON_WT *temp_vec2_ptr;
    int max_rowlen = pvec_pair->pvec2->max_partnum;
    int max_rowlen2 = pvec_pair->pvec1->max_partnum;
    short *hit_ptr;
    long i;
    long rownum, elt;
    RESULT_TUPLE *result_ptr;

    /*
     * Make space for storing the number of hits required.
     * No point in wasting the time if the match threshold is
     * really low.
     */
    if ( max_rowlen * max_rowlen2 > num_hits ) {
	if (num_hits != 0)
	    (void) free( (char *) hits );
	num_hits += max_rowlen * max_rowlen2;
	if (NULL == (hits = Malloc( num_hits, short )))
	    return UNDEF;
    }
    if (match_thresh > 1 )
	bzero( (char *) hits, max_rowlen*max_rowlen2 * sizeof(short) );

    PRINT_TRACE (2, print_string, "Trace: entering sim_inner_partvec_match");
    PRINT_TRACE (6, print_partvec, pvec_pair->pvec1);
    PRINT_TRACE (6, print_partvec, pvec_pair->pvec2);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "sim_inner_partvec_match");
        return (UNDEF);
    }

    vec1_ptr = pvec_pair->pvec1->part_con_wtp;
    vec1_end=&pvec_pair->pvec1->part_con_wtp[pvec_pair->pvec1->num_part_conwt];

    vec2_ptr = pvec_pair->pvec2->part_con_wtp;
    vec2_end=&pvec_pair->pvec2->part_con_wtp[pvec_pair->pvec2->num_part_conwt];

    if (vec1_ptr < vec1_end &&
        vec2_ptr < vec2_end &&
        info[inst].ctype_weight != 0.0) {

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
		temp_vec2_ptr = vec2_ptr;
		rownum = max_rowlen * vec1_ptr->partnum;

		while (temp_vec2_ptr < vec2_end &&
		       temp_vec2_ptr->con == vec2_ptr->con) {
		    elt = rownum + temp_vec2_ptr->partnum;
		    hits[elt]++;

		    if (pvec_results->res_ptr[elt] == NULL) {
			result_ptr = &pvec_results->results[
						 pvec_results->num_results++];
			pvec_results->res_ptr[elt] = result_ptr;

			result_ptr->sim = vec1_ptr->wt * temp_vec2_ptr->wt;
			result_ptr->qid = vec1_ptr->partnum;
			result_ptr->did = temp_vec2_ptr->partnum;
		    }
		    else
			pvec_results->res_ptr[elt]->sim
			    += vec1_ptr->wt * temp_vec2_ptr->wt;
		    temp_vec2_ptr++;
		}
                if (++vec1_ptr >= vec1_end) /* don't inc vec2_ptr */
                    break;
            }
        }
    }
    
    /* Enforce the match threshold */
    if ( match_thresh > 1 ) {
	hit_ptr = hits;
	for (i=0; i<max_rowlen*max_rowlen2; i++) {
	    if ( *hit_ptr > 0 && *hit_ptr < match_thresh )
		pvec_results->res_ptr[i]->sim = 0.0;
	    hit_ptr++;
	}
    }

    PRINT_TRACE (2, print_string, "Trace: leaving sim_inner_partvec_match");
    return (1);
}


int
close_sim_inner_partvec_match (inst)
int inst;
{
    PRINT_TRACE (2, print_string,
                 "Trace: entering/leaving close_sim_inner_partvec_match");
    return (0);
}
