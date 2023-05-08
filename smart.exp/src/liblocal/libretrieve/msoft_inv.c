#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/msoft_inv.c,v 11.0 1992/07/21 18:22:26 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Perform inverted retrieval requiring returned docs to satisfy criteria
 *1 retrieve.coll_sim.msoft_inv
 *2 msoft_inv (in_retrieval, results, inst)
 *3   RETRIEVAL *in_retrieval;
 *3   RESULT *results;
 *3   int inst;
 *4 init_msoft_inv (spec, unused)
 *5   "retrieve.num_wanted"
 *5   "retrieve.local.num_wanted"
 *5   "retrieve.local.sim"
 *5   "retrieve.local.combine"
 *5   "retrieve.doc.textloc_file"
 *5   "retrieve.coll_sim.trace"
 *5
 *5   "retrieve.local.sect_map"		From $I/fw_sect
 *5   "retrieve.local.sect_map.rmode"
 *5   "retrieve.local.sect_file"	From $I/fw_sect
 *5   "retrieve.local.sect_file.rmode"
 *5
 *5   "retrieve.local.docdoc_sim_thresh"
 *5					Only those retrieved docs in
 *5					the top local.num_wanted that
 *5					exceed this threshold are used
 *5					for the section breakdown.
 *5   "retrieve.local.sect_incr"	Value  toadd to sections to
 *5					distinguish from docid
 *5   "retrieve.local.sect_thresh"	if max section-section sim is
 *5					greater than sect_thresh,
 *5					return section id instead of docid.
 *5   "retrieve.local.sect_sims"	boolean: if sect id returned, use
 *5					sect sim rather than doc sim
 *5   "retrieve.local.unless_smaller"	if sect id used, use sect sim
 *5					ONLY if it's bigger than doc sim
 *5   "retrieve.local.sect_if_best"	boolean: if true, section id is
 *5					used if sect-sect match beats
 *5					doc-doc similarity, regardless
 *5					of sect_thresh value (sect sim
 *5					is used in that case).
 *4 close_msoft_inv (inst)

 *7 Retrieve up to num_wanted docs for the query in_retrieval->query.vector
 *7 which satisfy additional requirements as given by the procedure
 *7 designated by local.sim.  Done by initially retrieving
 *7 local.num_wanted documents using the normal inverted file approach.
 *7 The query is then compared with each of those docs in turn, using the
 *7 procedure given by local.sim.  That procedure will calculate a new
 *7 similarity between the query and doc.  The procedure local.combine
 *7 will then take the sets of local.num_wanted old sims and new sims,
 *7 and combine them into a set of num_wanted  document results.
 *7 Each of the retrieved docs is then examined to see if the doc should
 *7 be replaced by one of its included sections.  If so, the tr.did field
 *7 is set to local.sect_incr (100,000 for fw) + sect_number.
 *7
 *7 Note that evaluation using full ranking (eg an rr_file) cannot be
 *7 done using this retrieval method.  Most documents do not go through 
 *7 this additional local similarity; normally because it's too
 *7 expensive.
 *7 
 *7 See local_thresh() as an example of a local.sim.

 *8 Normal inverted search algorithm (see sim_vec_inv()), with a separate
 *8 inverted search being done for each ctype of the query vector.  Then
 *8 a sequential search among the best retrieved docs.
 *9 Is too specific for fw.
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

/* Changes from sim_vec_inv to msoft_inv:
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
static long sect_incr;
static float sect_thresh;
static int use_sect_sims;
static int use_sect_unless_smaller;
static int sect_if_best;
static char *sect_map_file;
static long sect_map_mode;
static char *sect_file;
static long sect_mode;
static float docdoc_sim_thresh;

static SPEC_PARAM spec_args[] = {
    "retrieve.num_wanted",       getspec_long,   (char *) &num_wanted,
    "retrieve.local.num_wanted", getspec_long,   (char *) &local_num_wanted,
    "retrieve.local.sim",        getspec_func,   (char *) &local_sim.ptab,
    "retrieve.local.combine",    getspec_func,   (char *) &combine.ptab,
    "retrieve.doc.textloc_file", getspec_dbfile, (char *) &textloc_file,
    "retrieve.local.sect_incr",  getspec_long,   (char *) &sect_incr,
    "retrieve.local.sect_thresh",  getspec_float,   (char *) &sect_thresh,
    "retrieve.local.sect_map",  getspec_dbfile,   (char *) &sect_map_file,
    "retrieve.local.sect_sims",	getspec_bool,	(char *) &use_sect_sims,
    "retrieve.local.unless_smaller", getspec_bool,
                                      (char *) &use_sect_unless_smaller,
    "retrieve.local.sect_if_best",getspec_bool,	(char *) &sect_if_best,
    "retrieve.local.sect_map.rmode",  getspec_filemode,(char *) &sect_map_mode,
    "retrieve.local.sect_file", getspec_dbfile,   (char *) &sect_file,
    "retrieve.local.sect_file.rmode",getspec_filemode,(char *) &sect_mode,
    "retrieve.local.docdoc_sim_thresh", getspec_float,
                                                  (char *)&docdoc_sim_thresh,
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

static int vecs_vecs_inst;
static int sect_map_fd;
static int sect_fd;
static int get_vec_list();
static void free_saved_vec_list();

int init_vecs_vecs(), vecs_vecs(), close_vecs_vecs();

int
init_msoft_inv (spec, unused)
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;
    long i;
    char param_prefix[PATH_LEN];

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_msoft_inv");

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
        set_error (errno, "", "msoft_inv");
        return (UNDEF);
    }

    /* Initialization for msoft post-processing */
    if (UNDEF == (vecs_vecs_inst = init_vecs_vecs (spec, (char *)NULL)))
        return (UNDEF);
    if (UNDEF == (sect_map_fd = open_vector (sect_map_file, sect_map_mode)))
        return (UNDEF);
    if (UNDEF == (sect_fd = open_vector (sect_file, sect_mode)))
        return (UNDEF);


    PRINT_TRACE (2, print_string, "Trace: leaving init_msoft_inv");
    return (0);
}

int
msoft_inv (in_retrieval, results, inst)
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    QUERY_VECTOR *query_vec = in_retrieval->query;
    VEC *qvec = (VEC *) (query_vec->vector);
    long i,j;
    VEC ctype_query;
    CON_WT *con_wtp;
    VEC dvec;
    VEC_PAIR vec_pair;

    VEC_LIST query_vec_list, doc_vec_list;
    VEC_LIST_PAIR vec_list_pair;
    float max_thresh;
    int max_index = 0;
    long sect_num;
    ALT_RESULTS alt_results;

    char temp[100];
    
    PRINT_TRACE (2, print_string, "Trace: entering msoft_inv");
    PRINT_TRACE( 1, print_string, sprintf( temp, "Working on doc %d",
					   qvec->id_num ));
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
    vec_pair.vec1 = qvec;
    vec_pair.vec2 = &dvec;
    for (i = 0; i < local_num_wanted && global_top_results[i].sim > 0.0; i++) {
        dvec.id_num = global_top_results[i].did;
        local_top_results[i].did = global_top_results[i].did;
        local_top_results[i].sim = global_top_results[i].sim;
        /* Should fill in actual doc vector, but for now skip since all
           local_sims ignore it */
        if (UNDEF == local_sim.ptab->proc (&vec_pair,
                                           &local_top_results[i].sim,
                                           local_sim.inst))
            return (UNDEF);
    }

    /* Now combine the two sets of results */
    if (UNDEF == combine.ptab->proc (local_top_results,
                                     results,
                                     combine.inst))
        return (UNDEF);

    /*
     * Discard any documents that are not at least reasonably
     * similar and also limit the maximum number retrieved.
     */
    for (i=0; i<results->num_top_results; i++)
	if ( results->top_results[i].sim < docdoc_sim_thresh ) {
	    results->num_top_results = i;
	    break;
	}
    if ( results->num_top_results > num_wanted )
	results->num_top_results = num_wanted;

    /* Now for each retrieved doc, see if the doc itself should be
       included, or if a section from the doc should be included instead */
    vec_list_pair.vec_list1 = &query_vec_list;
    vec_list_pair.vec_list2 = &doc_vec_list;

    /* Get the vec_list corresponding to the query sections */
    /* Question: Do we instead just want to run query itself? */
/*    if (UNDEF == get_vec_list (qvec->id_num, &query_vec_list))
 *        return (UNDEF);
 */
 /* Answer: Yes. */
    query_vec_list.id_num = qvec->id_num;
    query_vec_list.num_vec = 1;
    query_vec_list.vec = qvec;

    for (i = 0; i < results->num_top_results; i++) {
	if ( results->top_results[i].did == qvec->id_num )
	    continue;

	if (results->top_results[i].sim == 0.0)
	    break;

        /* Get the vec_list corresponding to the doc sections */
	/* Note that only non-empty sections are returned */
        if (UNDEF == get_vec_list (results->top_results[i].did, 
                                   &doc_vec_list))
            return (UNDEF);

        /* If there is more than one non-empty vector in the doc sections,
           Run all query sections against all doc sections */
        if (doc_vec_list.num_vec > 1) { 
            if (UNDEF == vecs_vecs (&vec_list_pair,
                                    &alt_results,
                                    vecs_vecs_inst))
                return (UNDEF);
        
            /* If one of the section-section sims is greater than sect_thresh
               Then replace tr[i].did with max section id. */
            max_thresh = 0.0;
            for (j = 0; j < alt_results.num_results; j++) {
                if (alt_results.results[j].sim > max_thresh) {
                    max_thresh = alt_results.results[j].sim;
                    max_index = j;
                }
            }

	    sect_num = alt_results.results[max_index].did
		- doc_vec_list.vec[0].id_num;
	    PRINT_TRACE( 1, print_string, sprintf( temp,
			   "  Max for did %d.%d is %4.2f (overall is %4.2f)",
			  results->top_results[i].did,
			  sect_num,
			  max_thresh,
			  results->top_results[i].sim ));

            if (max_thresh > sect_thresh) {
                results->top_results[i].did += sect_incr * (sect_num+1);

		if (use_sect_sims &&
		    ( alt_results.results[max_index].sim >
		               results->top_results[i].sim ||
		      !use_sect_unless_smaller ))
		    results->top_results[i].sim =
			alt_results.results[max_index].sim;
	    }
	    else if (sect_if_best && max_thresh>results->top_results[i].sim) {
                results->top_results[i].did += sect_incr * (sect_num+1);
		results->top_results[i].sim =
		    alt_results.results[max_index].sim;
	    }
        }

        free_saved_vec_list (&doc_vec_list);
    }

    free_saved_vec_list (&query_vec_list);

    /* Just output the results to stdout for now */
    PRINT_TRACE( 1, print_string, "  Results returned:" );
    for (i = 0; i < results->num_top_results; i++) {
        PRINT_TRACE( 1, print_string,
		    sprintf ( temp, "    %6d\t%4.2f\t%7d",
			     qvec->id_num,
			     results->top_results[i].sim,
			     results->top_results[i].did));
    }

    PRINT_TRACE (8, print_full_results, results);
    PRINT_TRACE (4, print_top_results, results);
    PRINT_TRACE (2, print_string, "Trace: leaving msoft_inv");
    return (1);
}


int
close_msoft_inv (inst)
int inst;
{
    long i;
    PRINT_TRACE (2, print_string, "Trace: entering close_msoft_inv");

    for (i = 0; i < doc_desc.num_ctypes; i++) {
        if (UNDEF == (doc_desc.ctypes[i].inv_sim->close_proc (ctype_inst[i])))
            return (UNDEF);
    }
    (void) free ((char *) top_results);
    (void) free ((char *) full_results);

    if (UNDEF == local_sim.ptab->close_proc (local_sim.inst) ||
        UNDEF == combine.ptab->close_proc (combine.inst))
        return (UNDEF);

    /* Close for msoft post-processing */
    if (UNDEF == close_vecs_vecs (vecs_vecs_inst) ||
        UNDEF == close_vector (sect_map_fd) ||
        UNDEF == close_vector (sect_fd))
        return (UNDEF);


    PRINT_TRACE (2, print_string, "Trace: leaving close_msoft_inv");
    return (0);
}


/* For now, just get vectors corresponding to sectors from already indexed 
   vectors.  May want to change later (use section preparssing and pp_veclist)
   in order to get different weightings. */
static int
get_vec_list (id_num, vec_list)
long id_num;
VEC_LIST *vec_list;
{
    int status;
    VEC map_vec;
    long i;

    vec_list->num_vec = 0;

    /* Look up id_num in the sector map */
    map_vec.id_num = id_num;
    if (1 != (status = seek_vector (sect_map_fd, &map_vec)) ||
        1 != (status = read_vector (sect_map_fd, &map_vec)))
        return (status);

    if (NULL == (vec_list->vec = (VEC *) 
                 malloc ((unsigned) map_vec.num_conwt * sizeof (VEC))))
        return (UNDEF);

    /* Get vector corresponding to each entry in map_vec */
    for (i = 0; i < map_vec.num_conwt; i++) {
        vec_list->vec[vec_list->num_vec].id_num = map_vec.con_wtp[i].con;
        if (1 == seek_vector (sect_fd, &vec_list->vec[vec_list->num_vec]) &&
            1 == read_vector (sect_fd, &vec_list->vec[vec_list->num_vec])) {
            /* Save a copy of current vector */
            if (UNDEF == save_vec (&vec_list->vec[vec_list->num_vec]))
                return (UNDEF);
            vec_list->num_vec++;
        }
    }
    return (1);
}


static void   
free_saved_vec_list (vec_list)
VEC_LIST *vec_list;
{
    long i;

    for (i = 0; i < vec_list->num_vec; i++) {
        (void) free_vec (&vec_list->vec[i]);
    }
    (void) free ((char *) vec_list->vec);
}
