#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/sim_part_inv.c,v 11.0 1992/07/21 18:23:58 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "retrieve.h"
#include "rel_header.h"
#include "vector.h"
#include "tr_vec.h"
#include "docdesc.h"

/*  Operates on vector lists rather than a single vector.
 *  Run num_wanted matches on each vector and consolidate the results
 *  into one long list that includes up to num-vectors * num-wanted
 *  top results */

/*  Perform inner product similarity function using inverted file to access */
/*  document weights.  Query weights given by query vectors.  Weight for */
/*  type of query term (ctype weight) is given by ret_spec file. */

/* Basic algorithm: malloc enough space to keep track of all similarities */
/*  of documents with direct access by did. */
/*  Top num_wanted similarites kept track of throughout. */

static long num_wanted;
static char *textloc_file;       /* Used to tell how many docs in collection */
static PROC_TAB *rank_tr;

static SPEC_PARAM spec_args[] = {
    "retrieve.num_wanted",       getspec_long, (char *) &num_wanted,
    "retrieve.doc.textloc_file", getspec_dbfile, (char *) &textloc_file,
    "retrieve.rank_tr",          getspec_func, (char *) &rank_tr,
    TRACE_PARAM ("retrieve.coll_sim.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_INDEX_DOCDESC doc_desc;
static float *full_results;
static short *already_used;
static TOP_RESULT *top_results;
static long max_did_in_coll;
static int rank_tr_inst;

static int *ctype_inst;

/* for storing the cumulated set of top-matches */
#define DEFAULT_NUM_VEC 10
static RESULT cum_results;
static long cum_num_vec;

int
init_sim_parts_inv (spec, unused)
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;
    long i;
    char param_prefix[PATH_LEN];

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_parts_inv");

    if (UNDEF == lookup_spec_docdesc (spec, &doc_desc))
        return (UNDEF);

    /* Reserve space for inst id of each of the ctype retrieval procs */
    if (NULL == (ctype_inst = (int *)
                 malloc ((unsigned) doc_desc.num_ctypes * sizeof (int))))
        return (UNDEF);

    /* Call all initialization procedures */
    for (i = 0; i < doc_desc.num_ctypes; i++) {
        (void) sprintf (param_prefix, "ctype.%ld.", i);
        if (UNDEF == (ctype_inst[i] = doc_desc.ctypes[i].inv_sim->
                      init_proc (spec, param_prefix)))
            return (UNDEF);
    }
    if (UNDEF == (rank_tr_inst = rank_tr->init_proc (spec, NULL))) {
        return (UNDEF);
    }

    /* Reserve space for top ranked documents */
    if (NULL == (top_results = (TOP_RESULT *) malloc ((unsigned) (num_wanted+1)
                                                      * sizeof (TOP_RESULT))))
        return (UNDEF);
    cum_num_vec = DEFAULT_NUM_VEC;
    if (NULL == (cum_results.top_results = (TOP_RESULT *)
		 malloc ((unsigned) ((num_wanted+1)
			 * sizeof (TOP_RESULT)
			 * cum_num_vec ))))
	return (UNDEF);
    cum_results.num_top_results = cum_num_vec * (num_wanted+1);
    cum_results.full_results = NULL;
    cum_results.num_full_results = 0;
    cum_results.qid = 0;


    /* Reserve space for all partial sims PLUS an "already-used" flag */
    if (NULL != (rh = get_rel_header (textloc_file)) && rh->max_primary_value)
        max_did_in_coll = rh->max_primary_value;
    else
        max_did_in_coll = MAX_DID_DEFAULT;

    if (NULL == (full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float)))) {
        set_error (errno, "", "sim_parts_inv");
        return (UNDEF);
    }
    if (NULL == (already_used = (short *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (short)))) {
        set_error (errno, "", "sim_parts_inv");
        return (UNDEF);
    }


    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_parts_inv");
    return (0);
}

int
sim_parts_inv (in_retrieval, results, inst)
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    QUERY_VECTOR *query_vec = in_retrieval->query;
    VEC_LIST *qvec_list = (VEC_LIST *) (query_vec->vector);
    VEC *qvec;
    long i, v;
    VEC ctype_query;
    CON_WT *con_wtp;

    PRINT_TRACE (2, print_string, "Trace: entering sim_parts_inv");
    PRINT_TRACE (6, print_vector, (VEC *) query_vec->vector);

    results->qid = query_vec->qid;
    results->top_results = top_results;
    results->num_top_results = num_wanted;
    results->full_results = full_results;
    results->num_full_results = max_did_in_coll + 1;

    bzero ((char *) already_used, 
	   sizeof (short) * (int) (max_did_in_coll+1));
    /* ensure there's enough room for all possible results */
    if ( qvec_list->num_vec > cum_num_vec ) {
	(void) free( (char *) cum_results.top_results );
	if (NULL == (cum_results.top_results = (TOP_RESULT *)
		     malloc ((unsigned) ((num_wanted+1)
			     * sizeof (TOP_RESULT)
			     * cum_num_vec ))))
        return (UNDEF);
	cum_results.num_top_results = cum_num_vec * (num_wanted+1);
    }
    bzero ((char *) cum_results.top_results,
	   (int) (sizeof( TOP_RESULT ) * cum_num_vec * (num_wanted+1)));

    /*
     * For each vector in the list, do a retrieval and stuff the
     * results into the cumulative top-results which is what will be
     * returned.
     */
    for ( v=1; v<qvec_list->num_vec; v++ ) {
	qvec = &qvec_list->vec[v];
	if ( qvec->num_ctype == 0 )	/* usually there are several empty */
	    continue;

	/* Ensure all values in full_results and top_results are 0 initially */
	bzero ((char *) full_results, 
	       sizeof (float) * (int) (max_did_in_coll+1));
	bzero ((char *) top_results, 
	       sizeof (TOP_RESULT) * (int) (num_wanted + 1));

	/* Handle docs that have been previously seen by setting sim */
	/* to large negative number */
	for (i = 0; i < in_retrieval->tr->num_tr; i++)
	    full_results[in_retrieval->tr->tr[i].did] = -1.0e8;

	/* Perform retrieval, sequentially going through query by ctype */
	ctype_query.id_num = qvec->id_num;
	ctype_query.num_ctype = 1;
	con_wtp = qvec->con_wtp;
	for (i = 0; i < doc_desc.num_ctypes; i++) {
	    if (qvec->num_ctype <= i)
		break;
	    if (qvec->ctype_len[i] <= 0)
		continue;

	    ctype_query.ctype_len = &qvec->ctype_len[i];
	    ctype_query.num_conwt = qvec->ctype_len[i];
	    ctype_query.con_wtp = con_wtp;
	    con_wtp += qvec->ctype_len[i];
	    if (UNDEF == doc_desc.ctypes[i].inv_sim->
		proc (&ctype_query, results, ctype_inst[i]))
		return (UNDEF);
	}

	/*
	 * Done with this query, stuff the results in our cumulative
	 * set.  We only put vectors in that aren't already there.
	 */
	for ( i=0; i<num_wanted && top_results[i].sim != 0.0 ; i++ ) {
	    if ( already_used[ top_results[i].did ] == 0 ) {
		already_used[ top_results[i].did ] = 1;
		if (UNDEF == rank_tr->proc( &top_results[i],
					    &cum_results,
					    rank_tr_inst))
		    return (UNDEF);
	    }
	}
    }

    /*
     * Reset the top-results output to point to the cumulative top-rank
     * Note that the full-results information only reflects the LAST
     * vector.
     */
    results->top_results = cum_results.top_results;
    results->num_top_results = qvec_list->num_vec * num_wanted;

    PRINT_TRACE (8, print_full_results, results);
    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving sim_parts_inv");
    return (1);
}


int
close_sim_parts_inv (inst)
int inst;
{
    long i;
    PRINT_TRACE (2, print_string, "Trace: entering close_sim_parts_inv");

    for (i = 0; i < doc_desc.num_ctypes; i++) {
        if (UNDEF == (doc_desc.ctypes[i].inv_sim->close_proc (ctype_inst[i])))
            return (UNDEF);
    }
    if (UNDEF == rank_tr->close_proc(rank_tr_inst))
	return (UNDEF);
    (void) free ((char *) top_results);
    (void) free ((char *) cum_results.top_results);
    (void) free ((char *) full_results);
    (void) free ((char *) already_used);
    
    PRINT_TRACE (2, print_string, "Trace: leaving close_sim_parts_inv");
    return (0);
}
