#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/sim_vec_inv.c,v 11.0 1992/07/21 18:23:59 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Match whole query versus doc collection using inverted search
 *1 retrieve.coll_sim.vec_inv
 *1 retrieve.coll_sim.inverted
 *2 sim_vec_inv (in_retrieval, results, inst)
 *3   RETRIEVAL *in_retrieval;
 *3   RESULT *results;
 *3   int inst;
 *4 init_sim_vec_inv (spec, unused)
 *5   "retrieve.num_wanted"
 *5   "retrieve.textloc_file"
 *5   "index.ctype.*.inv_sim"
 *5   "retrieve.coll_sim.trace"
 *4 close_sim_vec_inv (inst)

 *7 Take a vector query given by in_retrieval, and match it against every
 *7 document in doc_file in turn, calling a ctype dependant matching
 *7 function given by the procedure index.ctype.N.inv_sim for ctype N.
 *7 The similarity of all docs to the query is returned in
 *7 results->full_results.  The top similarities are kept track of by
 *7 inv_sim and are returned in results->top_results.
 *7 Return UNDEF if error, else 1.
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

/*  Perform inner product similarity function using inverted file to access */
/*  document weights.  Query weights given by query vector.  */

/* Basic algorithm: malloc enough space to keep track of all similarities */
/*  of documents with direct access by did. */
/*  Top num_wanted similarites kept track of throughout. */

static long num_wanted;
static char *textloc_file;      /* Used to tell how many docs in collection */

static SPEC_PARAM spec_args[] = {
    "retrieve.num_wanted",       getspec_long, (char *) &num_wanted,
    "retrieve.doc.textloc_file", getspec_dbfile, (char *) &textloc_file,
    TRACE_PARAM ("retrieve.coll_sim.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_INDEX_DOCDESC doc_desc;
static float *full_results;
static TOP_RESULT *top_results;
static long max_did_in_coll;

static int *ctype_inst;


int
init_sim_vec_inv (spec, unused)
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;
    long i;
    char param_prefix[PATH_LEN];

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_vec_inv");

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

    /* Reserve space for top ranked documents */
    if (NULL == (top_results = (TOP_RESULT *) malloc ((unsigned) (num_wanted+1)
                                                      * sizeof (TOP_RESULT))))
        return (UNDEF);

    /* Reserve space for all partial sims */
    if (NULL != (rh = get_rel_header (textloc_file)) && rh->max_primary_value)
        max_did_in_coll = rh->max_primary_value;
    else
        max_did_in_coll = MAX_DID_DEFAULT;

    if (NULL == (full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float)))) {
        set_error (errno, "", "sim_vec_inv");
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_vec_inv");
    return (0);
}

int
sim_vec_inv (in_retrieval, results, inst)
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    QUERY_VECTOR *query_vec = in_retrieval->query;
    VEC *qvec = (VEC *) (query_vec->vector);
    long i;
    VEC ctype_query;
    CON_WT *con_wtp;

    PRINT_TRACE (2, print_string, "Trace: entering sim_vec_inv");
    PRINT_TRACE (6, print_vector, (VEC *) query_vec->vector);

    results->qid = query_vec->qid;
    results->top_results = top_results;
    results->num_top_results = num_wanted;
    results->full_results = full_results;
    results->num_full_results = max_did_in_coll + 1;

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

    PRINT_TRACE (8, print_full_results, results);
    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving sim_vec_inv");
    return (1);
}


int
close_sim_vec_inv (inst)
int inst;
{
    long i;
    PRINT_TRACE (2, print_string, "Trace: entering close_sim_vec_inv");

    for (i = 0; i < doc_desc.num_ctypes; i++) {
        if (UNDEF == (doc_desc.ctypes[i].inv_sim->close_proc (ctype_inst[i])))
            return (UNDEF);
    }
    (void) free ((char *) top_results);
    (void) free ((char *) full_results);
    
    PRINT_TRACE (2, print_string, "Trace: leaving close_sim_vec_inv");
    return (0);
}
