#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/local_inv2.c,v 11.0 1992/07/21 18:22:26 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Perform inverted retrieval requiring returned docs to satisfy criteria
 *1 local.coll_sim.local_inv2
 *2 local_inv2 (in_retrieval, results, inst)
 *3   RETRIEVAL *in_retrieval;
 *3   RESULT *results;
 *3   int inst;
 *4 init_local_inv2 (spec, unused)
 *5   "retrieve.num_wanted"
 *5   "retrieve.local.num_wanted"
 *5   "retrieve.local.sim"
 *5   "retrieve.local.combine"
 *5   "retrieve.doc.textloc_file"
 *5   "retrieve.coll_sim.trace"
 *5   "retrieve.coll_sim.short_circuit" - if true, the calls to local.sim
 *5		are stopped as soon as retrieve.num_wanted docs are
 *5		returned with a non-zero similarity.  Note that this
 *5		is ONLY reasonable if local.sim does not generate a new
 *5		similarity for docs (in which case they really all need
 *5		to be scanned to get the new ordering).
 *4 close_local_inv2 (inst)

 *7 Same as retrieve.coll_sim.local_inv, but with the short-circuiting
 *7 parameter thrown in.
 *7
 *7 Retrieve num_wanted docs for the query in_retrieval->query.vector
 *7 which satisfy additional requirements as given by the procedure
 *7 designated by local.sim.  Done by initially retrieving
 *7 local.num_wanted documents using the normal inverted file approach.
 *7 The query is then compared with each of those docs in turn, using the
 *7 procedure given by local.sim.  That procedure will calculate a new
 *7 similarity between the query and doc.  The procedure local.combine
 *7 will then take the sets of local.num_wanted old sims and new sims,
 *7 and combine them in the final set of num_wanted  results.
 *7
 *7 Note that evaluation using full ranking (eg an rr_file) can not be
 *7 done using this retrieval method.  Most documents do not go through 
 *7 this additional local similarity; normally because it's too
 *7 expensive.
 *7 
 *7 See local_thresh() as an example of a local.sim.

 *8 Normal inverted search algorithm (see sim_vec_inv()), with a separate
 *8 inverted search being done for each ctype of the query vector.  Then
 *8 a sequential search among the best retrieved docs.
 *9 Replaces req_vec_inv.
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

/* Changes from sim_vec_inv to local_inv2:
   Retrieve local.num_wanted top docs as potential candidates for final
   top docs.
   For each potential candidate, local req.sim to calculate new sims between
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
static long local_num_wanted;
static PROC_INST local_sim;
static PROC_INST combine;
static char *textloc_file;       /* Used to tell how many docs in collection */
static long short_circuit;

static SPEC_PARAM spec_args[] = {
    "retrieve.num_wanted",       getspec_long,   (char *) &num_wanted,
    "retrieve.local.num_wanted", getspec_long,   (char *) &local_num_wanted,
    "retrieve.local.sim",        getspec_func,   (char *) &local_sim.ptab,
    "retrieve.local.combine",    getspec_func,   (char *) &combine.ptab,
    "retrieve.doc.textloc_file", getspec_dbfile, (char *) &textloc_file,
    "retrieve.coll_sim.short_circuit",
                                 getspec_long,	 (char *) &short_circuit,
    TRACE_PARAM ("retrieve.coll_sim.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_INDEX_DOCDESC doc_desc;
static float *full_results;
static TOP_RESULT *top_results;
static TOP_RESULT *global_top_results;
static TOP_RESULT *local_top_results;
static int max_did_in_coll;

static int *ctype_inst;


int
init_local_inv2 (spec, unused)
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;
    long i;
    char param_prefix[PATH_LEN];

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_local_inv2");

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
    if (UNDEF == (local_sim.inst = local_sim.ptab->init_proc (spec, NULL)))
        return (UNDEF);
    if (UNDEF == (combine.inst = combine.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    /* Reserve space for top ranked documents, both local and final */
    if (NULL == (top_results = (TOP_RESULT *)
                 malloc ((unsigned) (num_wanted+1) * sizeof (TOP_RESULT))) ||
        NULL == (global_top_results = (TOP_RESULT *)
                 malloc ((unsigned)(local_num_wanted+1)*sizeof (TOP_RESULT)))||
        NULL == (local_top_results = (TOP_RESULT *)
                 malloc ((unsigned) (local_num_wanted+1)*sizeof (TOP_RESULT))))
        return (UNDEF);

    /* Reserve space for all partial sims */
    if (NULL != (rh = get_rel_header (textloc_file)) && rh->max_primary_value)
        max_did_in_coll = rh->max_primary_value;
    else
        max_did_in_coll = MAX_DID_DEFAULT;

    if (NULL == (full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float)))) {
        set_error (errno, "", "local_inv2");
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_local_inv2");
    return (0);
}

int
local_inv2 (in_retrieval, results, inst)
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    QUERY_VECTOR *query_vec = in_retrieval->query;
    VEC *qvec = (VEC *) (query_vec->vector);
    long i;
    VEC ctype_query;
    CON_WT *con_wtp;
    VEC dvec;
    VEC_PAIR vec_pair;
    int nonzero;

    PRINT_TRACE (2, print_string, "Trace: entering local_inv2");
    PRINT_TRACE (6, print_vector, (VEC *) query_vec->vector);

    results->qid = query_vec->qid;
    results->full_results = full_results;
    results->num_full_results = max_did_in_coll + 1;
    /* Initially set top_results to get best local_num_wanted */
    results->top_results = global_top_results;
    results->num_top_results = local_num_wanted;

    /* Ensure all values in full_results and top_results are 0 initially */
    bzero ((char *) full_results, 
           sizeof (float) * (int) (max_did_in_coll+1));
    bzero ((char *) local_top_results, 
           sizeof (TOP_RESULT) * (int) (local_num_wanted + 1));
    bzero ((char *) global_top_results, 
           sizeof (TOP_RESULT) * (int) (local_num_wanted + 1));
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

    /* Go through local_num_wanted top docs, calculating a new sim for
       each, and then returning best num_wanted */
    nonzero = 0;   /* for short-circuiting */
    vec_pair.vec1 = qvec;
    vec_pair.vec2 = &dvec;
    for (i = 0; i < local_num_wanted && global_top_results[i].sim > 0.0; i++) {
        dvec.id_num = global_top_results[i].did;
        local_top_results[i].did = global_top_results[i].did;
        local_top_results[i].sim = global_top_results[i].sim;
        /* Should fill in actual doc vector, but for now skip since all
           local_sims ignore it */

	if (!short_circuit || nonzero < num_wanted ) {
	    if (UNDEF == local_sim.ptab->proc (&vec_pair,
					       &local_top_results[i].sim,
					       local_sim.inst))
		return (UNDEF);

	    if (local_top_results[i].sim != 0.0)
		nonzero++;
	}
	else
	    local_top_results[i].sim = 0.0;  /* just make 'em zero */
    }

    /* Now combine the two sets of results */
    if (UNDEF == combine.ptab->proc (local_top_results,
                                     results,
                                     combine.inst))
        return (UNDEF);

    /* And truncate to top num_wanted instead of local_num_wanted */
    results->num_top_results = num_wanted;

    PRINT_TRACE (8, print_full_results, results);
    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving local_inv2");
    return (1);
}


int
close_local_inv2 (inst)
int inst;
{
    long i;
    PRINT_TRACE (2, print_string, "Trace: entering close_local_inv2");

    for (i = 0; i < doc_desc.num_ctypes; i++) {
        if (UNDEF == (doc_desc.ctypes[i].inv_sim->close_proc (ctype_inst[i])))
            return (UNDEF);
    }
    (void) free ((char *) top_results);
    (void) free ((char *) full_results);

    if (UNDEF == local_sim.ptab->close_proc (local_sim.inst) ||
        UNDEF == combine.ptab->close_proc (combine.inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_local_inv2");
    return (0);
}
