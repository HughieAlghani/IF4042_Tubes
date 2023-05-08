#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/req_vec_inv.c,v 11.0 1992/07/21 18:22:33 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Perform inverted retrieval requiring returned docs to satisfy criteria
 *1 retrieve.coll_sim.req_inverted
 *2 req_vec_inv (in_retrieval, results, inst)
 *3   RETRIEVAL *in_retrieval;
 *3   RESULT *results;
 *3   int inst;
 *4 init_req_vec_inv (spec, unused)
 *5   "retrieve.rank_tr"
 *5   "retrieve.num_wanted"
 *5   "retrieve.required.num_wanted"
 *5   "retrieve.required.sim"
 *5   "retrieve.required.fill"
 *5   "retrieve.doc.textloc_file"
 *5   "retrieve.coll_sim.trace"
 *4 close_req_vec_inv (inst)

 *7 Retrieve num_wanted docs for the query in_retrieval->query.vector
 *7 which satisfy additional requirements as given by the procedure
 *7 designated by required.sim.  Done by initially retrieving
 *7 required.num_wanted documents using the normal inverted file approach.
 *7 The query is then compared with each of those docs in turn, using the
 *7 procedure given by required.sim.  That procedure will adjust the
 *7 similarity (frequently by adding or not adding a uniform increment
 *7 or changing the similarity to 0.0--e.g., if the doc does not meet some
 *7 criterion).  The top num_wanted docs within the reranked documents
 *7 (ranked by rank_tr) are then returned.
 *7
 *7 If fill is non-zero, and there are less than num_wanted docs satisfying
 *7 the criteria, then the best of the top docs that did not satisfy 
 *7 the criteria will fill in the rest of the open spots (the uniform
 *7 increment is assumed to be higher than all valid sims).  If fill is
 *7 zero, the empty spots are left unfilled.
 *7 
 *7 Note that evaluation using full ranking (eg an rr_file) can not be
 *7 done using this retrieval method.  Most documents do not go through 
 *7 this additional criteria checking; normally because it's too
 *7 expensive.
 *7 
 *7 See part_thresh() as an example of a required.sim.

 *8 Normal inverted search algorithm (see sim_vec_inv()), with a separate
 *8 inverted search being done for each ctype of the query vector.
 *9 Should be more general.
***********************************************************************/

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

/* Changes from sim_vec_inv to req_vec_inv:
   Retrieve req.num_wanted top docs as potential candidates for final
   top docs.
   For each potential candidate, run req.sim to calculate new sims between
   query and doc.
   Return top num_wanted candidates.
*/


/*  Perform inner product similarity function using inverted file to access */
/*  document weights.  Query weights given by query vector.  Weight for */
/*  type of query term (ctype weight) is given by ret_spec file. */

/* Basic algorithm: malloc enough space to keep track of all similarities */
/*  of documents with direct access by did. */
/*  Top num_wanted similarites kept track of throughout. */

static long num_wanted;
static long req_num_wanted;
static long fill_wanted;
static PROC_INST req_sim;
static PROC_INST rank_tr;
static char *textloc_file;       /* Used to tell how many docs in collection */

static SPEC_PARAM spec_args[] = {
    "retrieve.rank_tr",		 getspec_func, (char *) &rank_tr.ptab,
    "retrieve.num_wanted",       getspec_long, (char *) &num_wanted,
    "retrieve.required.num_wanted", getspec_long, (char *) &req_num_wanted,
    "retrieve.required.sim",     getspec_func,   (char *) &req_sim.ptab,
    "retrieve.required.fill",    getspec_long,   (char *) &fill_wanted,
    "retrieve.doc.textloc_file",    getspec_dbfile, (char *) &textloc_file,
    TRACE_PARAM ("retrieve.coll_sim.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_INDEX_DOCDESC doc_desc;
static float *full_results;
static TOP_RESULT *top_results;
static TOP_RESULT *req_top_results;
static int max_did_in_coll;

static int *ctype_inst;


int
init_req_vec_inv (spec, unused)
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;
    long i;
    char param_prefix[PATH_LEN];

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_req_vec_inv");

    if (UNDEF == lookup_spec_docdesc (spec, &doc_desc))
        return (UNDEF);

    /* Reserve space for inst id of each of the ctype retrieval procs */
    if (NULL == (ctype_inst = (int *)
                 malloc ((unsigned) doc_desc.num_ctypes * sizeof (int))))
        return (UNDEF);

    /* Call all initialization procedures */
    for (i = 0; i < doc_desc.num_ctypes; i++) {
        (void) sprintf (param_prefix, "ctype.%d.", i);
        if (UNDEF == (ctype_inst[i] = doc_desc.ctypes[i].inv_sim->
                      init_proc (spec, param_prefix)))
            return (UNDEF);
    }
    if (UNDEF == (req_sim.inst = req_sim.ptab->init_proc (spec, NULL)) ||
	UNDEF == (rank_tr.inst = rank_tr.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    /* Reserve space for top ranked documents, both required and final */
    if (NULL == (top_results = (TOP_RESULT *)
                 malloc ((unsigned) (num_wanted+1) * sizeof (TOP_RESULT))))
        return (UNDEF);
    if (NULL == (req_top_results = (TOP_RESULT *)
                 malloc ((unsigned) (req_num_wanted+1) * sizeof (TOP_RESULT))))
        return (UNDEF);

    /* Reserve space for all partial sims */
    if (NULL != (rh = get_rel_header (textloc_file)) && rh->max_primary_value)
        max_did_in_coll = rh->max_primary_value;
    else
        max_did_in_coll = MAX_DID_DEFAULT;

    if (NULL == (full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float)))) {
        set_error (errno, "", "req_vec_inv");
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_req_vec_inv");
    return (0);
}

int
req_vec_inv (in_retrieval, results, inst)
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    QUERY_VECTOR *query_vec = in_retrieval->query;
    VEC *qvec = (VEC *) (query_vec->vector);
    long i;
    VEC ctype_query;
    CON_WT *con_wtp;
    int num_good_matches;
    float new_sim;
    VEC dvec;
    VEC_PAIR vec_pair;
    TOP_RESULT new_top;

    PRINT_TRACE (2, print_string, "Trace: entering req_vec_inv");
    PRINT_TRACE (6, print_vector, (VEC *) query_vec->vector);

    results->qid = query_vec->qid;
    results->full_results = full_results;
    results->num_full_results = max_did_in_coll + 1;
    /* Initially set top_results to get best req_num_wanted */
    results->top_results = req_top_results;
    results->num_top_results = req_num_wanted;

    /* Ensure all values in full_results and top_results are 0 initially */
    bzero ((char *) full_results, 
           sizeof (float) * (int) (max_did_in_coll+1));
    bzero ((char *) req_top_results, 
           sizeof (TOP_RESULT) * (int) (req_num_wanted + 1));
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

    PRINT_TRACE (6, print_top_results, results);

    /* Point results to where we're be storing the reranked results */
    results->top_results = top_results;
    results->num_top_results = num_wanted;

    /* Go through req_num_wanted top docs, calculating a new sim for
       each, and then returning best num_wanted */
    vec_pair.vec1 = qvec;
    vec_pair.vec2 = &dvec;
    num_good_matches = 0;
    for (i = 0; i < req_num_wanted && req_top_results[i].sim > 0.0; i++) {
        dvec.id_num = req_top_results[i].did;
        new_sim = req_top_results[i].sim;

        /* Should fill in actual vector, but for now skip since all
           required_sims ignore it */
        if (UNDEF == req_sim.ptab->proc (&vec_pair,
                                         &new_sim,
                                         req_sim.inst))
            return (UNDEF);

	/* If the new similarity is non-zero, stick it in the top-ranked
	 * which we're building. */
        if (new_sim != 0.0) {
	    new_top.sim = new_sim;
	    new_top.did = req_top_results[i].did;

	    if (UNDEF == rank_tr.ptab->proc( &new_top,
					    results,
					    rank_tr.inst ))
		return (UNDEF);

            req_top_results[i].sim = 0.0; /* clear so not used as "fill" */
            num_good_matches++;
        }
 /* Don't do this optimization for now
  *    if (num_good_matches >= num_wanted)
  *          break;
  */
    }

    /* If fill_wanted, then the top docs not satisfying the criteria will
     * fill in any empty spots in top_results (else left as 0 entries)
     * This outa work--using the array offset rather than rank_tr, since
     * the array cannot possibly be full yet. */
    if (fill_wanted && num_good_matches < num_wanted) {
        for (i = 0; num_good_matches < num_wanted && i < req_num_wanted; i++) {
            if (req_top_results[i].sim > 0.0) {
                top_results[num_good_matches].did = req_top_results[i].did;
                top_results[num_good_matches].sim = req_top_results[i].sim;
                num_good_matches++;
            }
        }
    }
            
    PRINT_TRACE (8, print_full_results, results);
    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving req_vec_inv");
    return (1);
}


int
close_req_vec_inv (inst)
int inst;
{
    long i;
    PRINT_TRACE (2, print_string, "Trace: entering close_req_vec_inv");

    if (UNDEF == rank_tr.ptab->close_proc( rank_tr.inst ) ||
	UNDEF == req_sim.ptab->close_proc( req_sim.ptab ))
	return (UNDEF);

    for (i = 0; i < doc_desc.num_ctypes; i++) {
        if (UNDEF == (doc_desc.ctypes[i].inv_sim->close_proc (ctype_inst[i])))
            return (UNDEF);
    }
    (void) free ((char *) top_results);
    (void) free ((char *) full_results);
    
    PRINT_TRACE (2, print_string, "Trace: leaving close_req_vec_inv");
    return (0);
}
