#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/sim_seq.c,v 11.0 1992/07/21 18:23:43 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Match query versus doc collection using sequential search
 *1 retrieve.coll_sim.seq
 *2 sim_seq (in_retrieval, results, inst)
 *3   RETRIEVAL *in_retrieval;
 *3   RESULT *results;
 *3   int inst;
 *4 init_sim_seq (spec, unused)
 *5   "retrieve.num_wanted"
 *5   "retrieve.doc_file"
 *5   "retrieve.doc_file.rmode"
 *5   "retrieve.vec_vec"
 *5   "retrieve.rank_tr"
 *5   "retrieve.coll_sim.trace"
 *4 close_sim_seq (inst)

 *7 Take a vector query given by in_retrieval, and match it against every
 *7 document in doc_file in turn, using the matching function given by
 *7 vec_vec.  The similarity of all docs to the query is returned in
 *7 results->full_results.  The top similarities are kept track of by rank_tr,
 *7 and are returned in results->top_results.
 *7 Return UNDEF if error, else 1.

 *8 description of implementation
 *9 bugs and warnings
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

/*  Perform designated similarity function using sequential file to access */
/*  document vectors. */
/*  Top num_wanted similarites kept track of throughout. */

static long num_wanted;
static char *doc_file;
static long doc_mode;
static PROC_INST vec_vec;         /* Vector-vector similarity procedure */
static PROC_INST rank_tr;

static SPEC_PARAM spec_args[] = {
    "retrieve.num_wanted",       getspec_long,   (char *) &num_wanted,
    "retrieve.doc_file",         getspec_dbfile, (char *) &doc_file,
    "retrieve.doc_file.rmode",   getspec_filemode,(char *) &doc_mode,
    "retrieve.vec_vec",          getspec_func,   (char *) &vec_vec.ptab,
    "retrieve.rank_tr",          getspec_func,   (char *) &rank_tr.ptab,
    TRACE_PARAM ("retrieve.coll_sim.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static float *full_results;
static TOP_RESULT *top_results;
static long max_did_in_coll;
static int doc_fd;

static int num_inst = 0;

int
init_sim_seq (spec, unused)
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;

    if (num_inst++ > 0) {
        /* Assume all instantiations equal */
        PRINT_TRACE (2, print_string, "Trace: entering/leaving init_sim_seq");
        return (0);
    }

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_seq");

    /* Call all initialization procedures */
    if (UNDEF == (vec_vec.inst = vec_vec.ptab->init_proc (spec, NULL)))
        return (UNDEF);
    if (UNDEF == (rank_tr.inst = rank_tr.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    /* Open the doc_file */
    if (UNDEF == (doc_fd = open_vector (doc_file, doc_mode)))
        return (UNDEF);

    /* Reserve space for top ranked documents */
    if (NULL == (top_results = (TOP_RESULT *) malloc ((unsigned) (num_wanted+1)
                                                      * sizeof (TOP_RESULT))))
        return (UNDEF);

    /* Reserve space for all sims */
    if (NULL == (rh = get_rel_header (doc_file)))
        return (UNDEF);
    max_did_in_coll = rh->max_primary_value;
    if (NULL == (full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float)))) {
        set_error (errno, "", "sim_seq");
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_seq");
    return (0);
}

int
sim_seq (in_retrieval, results, inst)
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    QUERY_VECTOR *query_vec = in_retrieval->query;
    VEC *qvec = (VEC *) (query_vec->vector);
    VEC dvec;
    VEC_PAIR vec_pair;
    float *top_thresh;
    TOP_RESULT *new_top;
    TR_TUP *current_seen_ptr, *end_seen_ptr;
    long next_seen_doc;
    int status;

    PRINT_TRACE (2, print_string, "Trace: entering sim_seq");
    PRINT_TRACE (6, print_vector, (VEC *) query_vec->vector);

    results->qid = query_vec->qid;
    results->top_results = top_results;
    results->num_top_results = num_wanted;
    results->full_results = full_results;
    results->num_full_results = max_did_in_coll + 1;

    top_thresh = &(results->top_results[results->num_top_results-1].sim);
    new_top = &results->top_results[results->num_top_results];

    /* Ensure all values in full_results and top_results are 0 initially */
    bzero ((char *) full_results, 
           sizeof (float) * (int) (max_did_in_coll+1));
    bzero ((char *) top_results, 
           sizeof (TOP_RESULT) * (int) (num_wanted + 1));

    /* set next_seen_doc to the smallest docid that has been previously
       seen.  Assumes that in_retrieval->tr->tr sorted in increasing
       doc_id order */
    current_seen_ptr = in_retrieval->tr->tr;
    end_seen_ptr = in_retrieval->tr->num_tr ?
        &in_retrieval->tr->tr[in_retrieval->tr->num_tr] : current_seen_ptr;
    next_seen_doc = (current_seen_ptr < end_seen_ptr) ?
        current_seen_ptr->did : MAXLONG;
    
    /* Perform retrieval, sequentially going through doc collection */
    vec_pair.vec1 = qvec;
    vec_pair.vec2 = &dvec;
    if (UNDEF == seek_vector (doc_fd, (VEC *) NULL))
        return (UNDEF);

    while (1 == (status = read_vector (doc_fd, &dvec))) {
        /* Skip over dvec if in previously seen docs */
        if (dvec.id_num >= next_seen_doc) {
            while (current_seen_ptr < end_seen_ptr &&
                   dvec.id_num > current_seen_ptr->did)
                current_seen_ptr++;
            if (current_seen_ptr < end_seen_ptr &&
                dvec.id_num == current_seen_ptr->did) {
                current_seen_ptr++;
                next_seen_doc = (current_seen_ptr < end_seen_ptr) ?
                    current_seen_ptr->did : MAXLONG;
                continue;
            }
            next_seen_doc = (current_seen_ptr < end_seen_ptr) ?
                current_seen_ptr->did : MAXLONG;
        }
        
        /* Get similarity of qvec to dvec */
        if (UNDEF == vec_vec.ptab->proc (&vec_pair,
                                         &new_top->sim,
                                         vec_vec.inst))
            return (UNDEF);
        full_results[dvec.id_num] = new_top->sim;

        /* Add to top results (potentially) if new sim exceeds top_thresh*/
        /* (new_top points to last entry in top_results subarray of */
        /* results) */
        if (new_top->sim >= *top_thresh) {
            new_top->did = dvec.id_num;
            if (UNDEF == rank_tr.ptab->proc (new_top, results, rank_tr.inst))
                return (UNDEF);
        }
    }
    if (status == UNDEF)
        return (status);

    PRINT_TRACE (8, print_full_results, results);
    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving sim_seq");
    return (1);
}


int
close_sim_seq (inst)
int inst;
{
    if (--num_inst) {
        PRINT_TRACE (2, print_string, "Trace: entering/leaving close_sim_seq");
        return (0);
    }

    PRINT_TRACE (2, print_string, "Trace: entering close_sim_seq");
    
    if (UNDEF == vec_vec.ptab->close_proc (vec_vec.inst) ||
        UNDEF == rank_tr.ptab->close_proc (rank_tr.inst) ||
        UNDEF == close_vector (doc_fd))
        return (UNDEF);

    (void) free ((char *) top_results);
    (void) free ((char *) full_results);
    
    PRINT_TRACE (2, print_string, "Trace: leaving close_sim_seq");
    return (0);
}

