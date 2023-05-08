#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/sim_partvecs.c,v 11.0 1992/07/21 18:22:35 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Calculate similarity between pair of part vectors
 *1 retrieve.vecs_vecs.partvec_partvec
 *2 sim_partvecs (pvec_pair, sim_ptr, inst)
 *3   PART_VEC_PAIR *vec_pair;
 *3   ALT_RESULTS *result_list;
 *3   int inst;
 *4 init_sim_partvecs (spec, unused)
 *5   "retrieve.vec_vec.trace"
 *5   "index.ctype.*.partvec_sim"
 *4 close_sim_partvecs (inst)

 *7 Calculate the similarity between the two part vectors (given by pvec_pair)
 *7 by calling a ctype dependant similarity function given by
 *7 index.ctype.N.partvec_sim for each ctype N occurring in the vectors.
 *7 The partvec_sim routine called with prefix "ctype.N."

 *9 Note: Assumes similarity is 0.0 between ctypes of the vectors if
 *9 that ctype is not included in one or both of the vectors.
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
#include "part_vector.h"
#include "docdesc.h"

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("retrieve.vec_vec.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_INDEX_DOCDESC doc_desc;

static PROC_INST *partvec_sim;

static int num_inst = 0;

static int max_num_results = 0;
static RESULT_TUPLE *results;
static int max_num_ptrs = 0;
static RESULT_TUPLE **result_pointers;

#define Malloc(n,type) (type *) malloc( (unsigned) (n)*sizeof(type) )
static int compare_result_tuple();

int
init_sim_partvecs (spec, unused)
SPEC *spec;
char *unused;
{
    long i;
    char param_prefix[PATH_LEN];

    if (num_inst++ > 0) {
        /* Assume all instantiations equal */
        PRINT_TRACE (2, print_string,
                     "Trace: entering/leaving init_sim_partvecs");
        return (0);
    }

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_partvecs");

    if (UNDEF == lookup_spec_docdesc (spec, &doc_desc))
        return (UNDEF);

    /* Reserve space for inst id of each of the ctype retrieval procs */
    if (NULL == (partvec_sim = Malloc( doc_desc.num_ctypes, PROC_INST )))
	return UNDEF;
    
    /* Call all initialization procedures */
    for (i = 0; i < doc_desc.num_ctypes; i++) {
	SPEC_PARAM spec_generic;
	char buf[PATH_LEN];

	(void) sprintf( buf, "index.ctype.%d.partvec_sim", i );
	spec_generic.param = buf;
        spec_generic.result = (char *) &partvec_sim[i].ptab;
        spec_generic.convert = getspec_func;
        if (UNDEF == lookup_spec (spec, &spec_generic, 1))
            return (UNDEF);

        (void) sprintf (param_prefix, "ctype.%ld.", i);
        if (UNDEF == (partvec_sim[i].inst =
		          partvec_sim[i].ptab->init_proc( spec,
							  param_prefix )))
            return (UNDEF);
    }
    
    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_partvecs");
    return (0);
}

int
sim_partvecs (pvec_pair, res_list, inst)
PART_VEC_PAIR *pvec_pair;
ALT_RESULTS *res_list;
int inst;
{
    PART_VEC *qvec = pvec_pair->pvec1;
    PART_VEC *dvec = pvec_pair->pvec2;
    long i;
    PART_VEC ctype_query;
    PART_VEC ctype_doc;
    PART_VEC_PAIR ctype_pvec_pair;
    PART_CON_WT *query_part_con_wtp;
    PART_CON_WT *doc_part_con_wtp;
    PART_VEC_RESULTS pvec_results;

    PRINT_TRACE (2, print_string, "Trace: entering sim_partvecs");
    PRINT_TRACE (6, print_partvec, qvec);
    PRINT_TRACE (6, print_partvec, dvec);

    /*
     * We save enough space for every possible match.  Note that
     * the results are only filled up with non-zero entries.
     */
    if ( max_num_results < qvec->partnums_used*dvec->partnums_used ) {
	if ( max_num_results != 0 )
	    (void) free( (char *) results );
	max_num_results += qvec->partnums_used * dvec->partnums_used;
	if (NULL == (results = Malloc( max_num_results, RESULT_TUPLE )))
	    return UNDEF;
    }
    if ( max_num_ptrs < qvec->max_partnum*dvec->max_partnum ) {
	if ( max_num_ptrs != 0 )
	    (void) free( (char *) result_pointers );
	max_num_ptrs += qvec->max_partnum*dvec->max_partnum;
	if (NULL == (result_pointers = Malloc( max_num_ptrs, RESULT_TUPLE * )))
	    return UNDEF;
    }
    pvec_results.num_results = 0;
    pvec_results.results = results;
    pvec_results.res_ptr = result_pointers;
    bzero( (char *) result_pointers,
	   (int) qvec->max_partnum*dvec->max_partnum * sizeof(RESULT_TUPLE *));

    /* Optimize case where both doc and query have one ctype */
    if (qvec->num_ctype == 1 && dvec->num_ctype == 1) {
        if (UNDEF == partvec_sim[0].ptab->proc( pvec_pair,
					        &pvec_results,
					        partvec_sim[0].inst ))
            return (UNDEF);
    }
    else {
        /* Perform retrieval, sequentially going through query by ctype */
        ctype_query.id_num = qvec->id_num;
        ctype_query.num_ctype = 1;
        query_part_con_wtp = qvec->part_con_wtp;

        ctype_doc.id_num = dvec->id_num;
        ctype_doc.num_ctype = 1;
        doc_part_con_wtp = dvec->part_con_wtp;

        ctype_pvec_pair.pvec1 = &ctype_query;
        ctype_pvec_pair.pvec2 = &ctype_doc;
        
        for (i = 0; i < doc_desc.num_ctypes; i++) {
            if (qvec->num_ctype <= i || dvec->num_ctype <= i)
                break;
            if (qvec->ctype_len[i] <= 0 || dvec->ctype_len[i] <= 0) {
                query_part_con_wtp += qvec->ctype_len[i];
                doc_part_con_wtp += dvec->ctype_len[i];
                continue;
            }
            
	    /* This ctype occurs in both query and document */
            ctype_query.ctype_len = &qvec->ctype_len[i];
            ctype_query.num_part_conwt = qvec->ctype_len[i];
            ctype_query.part_con_wtp = query_part_con_wtp;
            query_part_con_wtp += qvec->ctype_len[i];

            ctype_doc.ctype_len = &dvec->ctype_len[i];
            ctype_doc.num_part_conwt = dvec->ctype_len[i];
            ctype_doc.part_con_wtp = doc_part_con_wtp;
            doc_part_con_wtp += dvec->ctype_len[i];

	    if (UNDEF == partvec_sim[i].ptab->proc( &ctype_pvec_pair,
						    &pvec_results,
						    partvec_sim[i].inst ))
                return (UNDEF);
        }
    }

    qsort( (char *) pvec_results.results,
	   (int) pvec_results.num_results,
	   sizeof( RESULT_TUPLE ),
	   compare_result_tuple );
    res_list->num_results = pvec_results.num_results;
    res_list->results = pvec_results.results;
    
    PRINT_TRACE (2, print_string, "Trace: leaving sim_partvecs");
    return (1);
}


int
close_sim_partvecs (inst)
int inst;
{
    long i;
 
    if (--num_inst) {
        PRINT_TRACE (2, print_string,
                     "Trace: entering/leaving close_sim_partvecs");
        return (0);
    }

    PRINT_TRACE (2, print_string, "Trace: entering close_sim_partvecs");

    if ( max_num_results != 0 )
	(void) free( (char *) results );

    for (i = 0; i < doc_desc.num_ctypes; i++) {
	if (UNDEF == partvec_sim[i].ptab->close_proc( partvec_sim[i].inst ))
            return (UNDEF);
    }

    (void) free ((char *) partvec_sim );

    PRINT_TRACE (2, print_string, "Trace: leaving close_sim_partvecs");
    return (0);
}


/******************************************************************
 *
 * Sort a RESULT_TUPLE by increasing did within increasing qid,
 * within decreasing similarity.
 *
 ******************************************************************/
static int
compare_result_tuple( tup1, tup2 )
RESULT_TUPLE *tup1, *tup2;
{
    if ( tup1->sim > tup2->sim )
	return -1;
    if ( tup1->sim < tup2->sim )
	return 1;

    if ( tup1->qid < tup2->qid )
	return -1;
    if ( tup1->qid > tup2->qid )
	return 1;

    if ( tup1->did < tup2->did )
	return -1;
    if ( tup1->did > tup2->did )
	return 1;

    return 0;
}
