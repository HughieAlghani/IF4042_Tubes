#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/sim_fw_parts.c,v 11.0 1992/07/21 18:22:29 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Match query versus doc coll usings syntactic parts of vectors for fw coll
 *1 retrieve.coll_sim.fw_parts
 *2 sim_fw_parts (in_retrieval, results, inst)
 *3   RETRIEVAL *in_retrieval;
 *3   RESULT *results;
 *3   int inst;
 *4 init_sim_fw_parts (spec, unused)
 *5   "retrieve.num_wanted"
 *5   "retrieve.textloc_file"
 *5   "index.ctype.*.inv_sim"
 *5   "parts.query_file"
 *5   "parts.doc_file"
 *5   "parts.id_map"
 *5   "retrieve.rank_parts"
 *5   "retrieve.coll_sim.trace"
 *4 close_sim_fw_parts (inst)

 *7 Take a vector query given by in_retrieval, and match it against every
 *7 document in doc_file in turn, calling a ctype dependant matching
 *7 function given by the procedure index.ctype.N.inv_sim for ctype N.
 *7 The similarity of all docs to the query is returned in
 *7 results->full_results.  The top similarities are returned 
 *7 in results->top_results.
 *7 Return UNDEF if error, else 1.

For each query vector, ignore the vector itself.  Take the qid, look it up
in id_map.  
  Foreach part id_num given by id_map, look it up in query_file, 
  getting a vector. Run inverted search on that vector, using 
  sim_ctype_inv on parts.inv_file (param_prefix "parts." given to
  init_sim_ctype_inv).
Reserve space for
  Final full_results for documents.
  Parts_full_results for parts search.  Space needed given by parts.doc_file.
  max_full_results gives the max parts-parts sim seen.
  sum_full_results gives the sum of the parts-parts sim seen.
  num_parts gives the number of parts in each doc.
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
#include "buf.h"

/*  Perform inner product similarity function using inverted file to access */
/*  document weights.  Query weights given by query vector.  */

/* Basic algorithm: malloc enough space to keep track of all similarities */
/*  of documents with direct access by did. */
/*  Top num_wanted similarites kept track of throughout. */

static long num_wanted;
static char *textloc_file;       /* Used to tell how many docs in collection */
static char *query_file;         /* Query file for parts vectors */
static long query_mode;
static char *doc_file;           /* Doc file for parts vectors */
static char *parts_map;          /* Vector file giving map of docid to partid*/
static long parts_mode;
static PROC_INST rank_parts;     /* Procedure to determine final doc sim
                                    based on all parts sims. */

static SPEC_PARAM spec_args[] = {
    "retrieve.num_wanted",       getspec_long, (char *) &num_wanted,
    "retrieve.doc.textloc_file", getspec_dbfile, (char *) &textloc_file,
    "parts.query_file",          getspec_dbfile, (char *) &query_file,
    "parts.query_file.rmode",    getspec_filemode, (char *) &query_mode,
    "parts.doc_file",            getspec_dbfile, (char *) &doc_file,
    "parts.id_map",              getspec_dbfile, (char *) &parts_map,
    "parts.id_map.rmode",        getspec_filemode, (char *) &parts_mode,
    "retrieve.rank_parts",       getspec_func,   (char *) &rank_parts.ptab,
    TRACE_PARAM ("retrieve.coll_sim.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static SM_INDEX_DOCDESC doc_desc;
static float *full_results;      /* full_results[did] gives final sim */
static float *max_full_results;  /* max_full_results[did] gives max part sim */
static float *sum_full_results;  /* sum_full_results[did] gives sum part sim */
static int *num_parts;           /* num_parts[did] gives number parts in did */
static float *parts_full_results;/* parts_full_results[partid] gives sim */
static int *parts_did;           /* parts_did[partid] gives did of part */

static TOP_RESULT *top_results;
static long max_did_in_coll;
static long max_partid_in_coll;

static int query_fd, partid_fd;
static int *ctype_inst;

static void print_other_results(), print_parts_top_results();

int
init_sim_fw_parts (spec, unused)
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;
    long i;
    char param_prefix[PATH_LEN];
    VEC id_vec;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_sim_fw_parts");

    if (UNDEF == lookup_spec_docdesc (spec, &doc_desc))
        return (UNDEF);

    /* Reserve space for inst id of each of the ctype retrieval procs */
    if (NULL == (ctype_inst = (int *)
                 malloc ((unsigned) doc_desc.num_ctypes * sizeof (int))))
        return (UNDEF);

    /* Call all initialization procedures */
    for (i = 0; i < doc_desc.num_ctypes; i++) {
        (void) sprintf (param_prefix, "parts.%d.", i);
        if (UNDEF == (ctype_inst[i] = doc_desc.ctypes[i].inv_sim->
                      init_proc (spec, param_prefix)))
            return (UNDEF);
    }
    if (UNDEF == (rank_parts.inst = rank_parts.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    /* Reserve space for top ranked documents */
    if (NULL == (top_results = (TOP_RESULT *) malloc ((unsigned) (num_wanted+1)
                                                      * sizeof (TOP_RESULT))))
        return (UNDEF);

    /* Reserve space for all partial sims */
    if (NULL != (rh = get_rel_header (textloc_file)) && rh->max_primary_value)
        max_did_in_coll = rh->max_primary_value;
    else
        max_did_in_coll = MAX_DID_DEFAULT;
    if (NULL != (rh = get_rel_header (doc_file)) && rh->max_primary_value)
        max_partid_in_coll = rh->max_primary_value;
    else
        max_partid_in_coll = MAX_DID_DEFAULT;

    if (NULL == (full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float))) ||
        NULL == (max_full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float))) ||
        NULL == (sum_full_results = (float *) 
                 malloc ((unsigned) (max_did_in_coll + 1) * sizeof (float))) ||
        NULL == (num_parts = (int *) 
                 calloc ((unsigned) (max_did_in_coll + 1), sizeof (int))) ||
        NULL == (parts_full_results = (float *) 
                 malloc ((unsigned) (max_partid_in_coll + 1)
                         * sizeof (float))) ||
        NULL == (parts_did = (int *) 
                 calloc ((unsigned) (max_partid_in_coll + 1), sizeof (int)))) {
        set_error (errno, "Malloc", "sim_fw_parts");
        return (UNDEF);
    }

    if (UNDEF == (query_fd = open_vector (query_file, query_mode)) ||
        UNDEF == (partid_fd = open_vector (parts_map, parts_mode)))
        return (UNDEF);

    while (1 == read_vector (partid_fd, &id_vec)) {
        for (i = 0; i < id_vec.num_conwt; i++) {
            parts_did[id_vec.con_wtp[i].con] = id_vec.id_num;
        }
        num_parts[id_vec.id_num] = id_vec.num_conwt;
    }
    
    PRINT_TRACE (2, print_string, "Trace: leaving init_sim_fw_parts");
    return (0);
}

int
sim_fw_parts (in_retrieval, results, inst)
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    long qid = in_retrieval->query->qid;
    long i, j, k;
    VEC ctype_query;
    CON_WT *con_wtp;
    VEC id_vec, part_vec;
    RESULT partial;
    PARTS_RESULTS parts_partial;

    PRINT_TRACE (2, print_string, "Trace: entering sim_fw_parts");
    PRINT_TRACE (6, print_long, &qid);

    results->qid = qid;
    results->top_results = top_results;
    results->num_top_results = num_wanted;
    results->full_results = full_results;
    results->num_full_results = max_did_in_coll + 1;

    /* Ensure all values in full_results and top_results are 0 initially */
    bzero ((char *) full_results, 
           sizeof (float) * (int) (max_did_in_coll+1));
    bzero ((char *) max_full_results, 
           sizeof (float) * (int) (max_did_in_coll+1));
    bzero ((char *) sum_full_results, 
           sizeof (float) * (int) (max_did_in_coll+1));
    bzero ((char *) top_results, 
           sizeof (TOP_RESULT) * (int) (num_wanted + 1));

    /* Handle docs that have been previously seen by setting sim */
    /* to large negative number */
    for (i = 0; i < in_retrieval->tr->num_tr; i++) {
        full_results[in_retrieval->tr->tr[i].did] = -1.0e8;
        max_full_results[in_retrieval->tr->tr[i].did] = -1.0e8;
        sum_full_results[in_retrieval->tr->tr[i].did] = -1.0e8;
    }

    /* Get each of the parts_vectors */
    id_vec.id_num = qid;
    if (1 != seek_vector (partid_fd, &id_vec) ||
        1 != read_vector (partid_fd, &id_vec))
        return (UNDEF);

    for (k = 0; k < id_vec.num_conwt; k++) {
        part_vec.id_num = id_vec.con_wtp[k].con;
        if (1 != seek_vector (query_fd, &part_vec) ||
            1 != read_vector (query_fd, &part_vec))
            return (UNDEF);

        /* Initialize part search */
        bzero ((char *) parts_full_results, 
               sizeof (float) * (int) (max_partid_in_coll+1));
        bzero ((char *) top_results, 
               sizeof (TOP_RESULT) * (int) (num_wanted + 1));
        partial.qid = part_vec.id_num;
        partial.top_results = top_results;
        partial.num_top_results = num_wanted;
        partial.full_results = parts_full_results;
        partial.num_full_results = max_partid_in_coll + 1;

        /* Perform retrieval, sequentially going through query by ctype */
        ctype_query.id_num = part_vec.id_num;
        ctype_query.num_ctype = 1;
        con_wtp = part_vec.con_wtp;
        for (i = 0; i < doc_desc.num_ctypes; i++) {
            if (part_vec.num_ctype <= i)
                break;
            if (part_vec.ctype_len[i] <= 0)
                continue;

            ctype_query.ctype_len = &part_vec.ctype_len[i];
            ctype_query.num_conwt = part_vec.ctype_len[i];
            ctype_query.con_wtp = con_wtp;
            con_wtp += part_vec.ctype_len[i];
            if (UNDEF == doc_desc.ctypes[i].inv_sim->
                proc (&ctype_query, &partial, ctype_inst[i]))
                return (UNDEF);
            PRINT_TRACE (6, print_parts_top_results, &partial);
        }

        /* Add parts results to full results */
        for (j = 0; j < max_partid_in_coll; j++) {
            if (parts_full_results[j] > 0.0) {
                sum_full_results[parts_did[j]] += parts_full_results[j];
                if (max_full_results[parts_did[j]] < parts_full_results[j] &&
                    max_full_results[parts_did[j]] >= 0.0)
                    max_full_results[parts_did[j]] = parts_full_results[j];
            }
        }
    }

    /* Compute the final results from all the partial data */
    parts_partial.max_doc = max_did_in_coll + 1;
    parts_partial.max_parts = max_partid_in_coll + 1;
    parts_partial.sum_results = sum_full_results;
    parts_partial.max_results = max_full_results;
    parts_partial.num_parts = num_parts;


    bzero ((char *) top_results, 
           sizeof (TOP_RESULT) * (int) (num_wanted + 1));
    if (UNDEF == rank_parts.ptab->proc (&parts_partial, 
                                        results,
                                        rank_parts.inst))
        return (UNDEF);
    

    PRINT_TRACE (20, print_other_results, &parts_partial);
    PRINT_TRACE (20, print_string, "Final results:");
    PRINT_TRACE (20, print_full_results, results);
/*    PRINT_TRACE (4, print_top_results, results); Temp hack for Salton */
    PRINT_TRACE (8, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving sim_fw_parts");
    return (1);
}


int
close_sim_fw_parts (inst)
int inst;
{
    long i;
    PRINT_TRACE (2, print_string, "Trace: entering close_sim_fw_parts");

    for (i = 0; i < doc_desc.num_ctypes; i++) {
        if (UNDEF == (doc_desc.ctypes[i].inv_sim->close_proc (ctype_inst[i])))
            return (UNDEF);
    }
    if (UNDEF == rank_parts.ptab->close_proc (rank_parts.inst))
        return (UNDEF);

    (void) free ((char *) ctype_inst);
    (void) free ((char *) top_results);
    (void) free ((char *) full_results);
    (void) free ((char *) max_full_results);
    (void) free ((char *) sum_full_results);
    (void) free ((char *) parts_full_results);
    (void) free ((char *) num_parts);
    (void) free ((char *) parts_did);


    PRINT_TRACE (2, print_string, "Trace: leaving close_sim_fw_parts");
    return (0);
}

static void
print_parts_top_results (partial, unused)
RESULT *partial;
SM_BUF *unused;
{
    long i;
    (void) printf ("Qid\tSectid\t(Docid)\tSim\n");
    for (i = 0; i < partial->num_top_results; i++) {
        if (partial->top_results[i].sim > 0.0) {
            (void) printf ("%d\t%d\t(%d)\t%6.4f\n",
                    partial->qid,
                    partial->top_results[i].did,
                    parts_did[partial->top_results[i].did],
                    partial->top_results[i].sim);
        }
    }
    
}

static void
print_other_results (parts_partial, unused)
PARTS_RESULTS *parts_partial;
SM_BUF *unused;
{
    long i;
    (void) printf ("Did\tSum\tMax\tLength\n");
    for (i = 0; i < parts_partial->max_doc; i++) {
        (void) printf ("%d\t%6.4f\t%6.4f\t%d\n",
                i,
                parts_partial->sum_results[i],
                parts_partial->max_results[i],
                parts_partial->num_parts[i]);
    }
    (void) fflush (stdout);
}
