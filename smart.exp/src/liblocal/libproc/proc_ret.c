#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libproc/proc_ret.c,v 11.0 1992/07/21 18:22:22 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "proc.h"

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to match a query against a collection
 *1 local.retrieve.coll_sim
 *2 * (in_retrieval, results, inst)
 *3   RETRIEVAL *in_retrieval;
 *3   RESULT *results;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take an input query (possibly along with a list of
 *7 docs NOT to retrieve) and compute the similarity between the query and
 *7 all docs in the collection.  Results->full_results gives all similarities,
 *7 results->top_results gives the top documents retrieved.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are  "req_inverted", "fw_parts", "local_inv2", 
 *8 "msoft_inv"
***********************************************************************/
/* Run a vector against entire collection */
extern int init_req_vec_inv(), req_vec_inv(), close_req_vec_inv();
extern int init_local_inv2(), local_inv2(), close_local_inv2();
extern int init_sim_fw_parts(), sim_fw_parts(), close_sim_fw_parts();
extern int init_msoft_inv(), msoft_inv(), close_msoft_inv();
static PROC_TAB proc_coll_sim[] = {
    "req_inverted",	init_req_vec_inv,   req_vec_inv,   close_req_vec_inv,
    "local_inv2",	init_local_inv2,     local_inv2,     close_local_inv2,
    "fw_parts",		init_sim_fw_parts,  sim_fw_parts,  close_sim_fw_parts,
    "msoft_inv",	init_msoft_inv,	    msoft_inv,	   close_msoft_inv,
    };
static int num_proc_coll_sim = sizeof (proc_coll_sim) / sizeof (proc_coll_sim[0]);

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to match a vector against a vector
 *1 local.retrieve.vec_vec
 *2 * (vec_pair, sim_ptr, inst)
 *3   VEC_PAIR *vec_pair;
 *3   float *sim_ptr;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a vector pair and computes the similarity between
 *7 the two vectors, returning the value in sim_ptr.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are  "part_rerank", "local_partvec"
***********************************************************************/
/* Run a vector against a vector, returning a sim */
extern int init_part_rerank(), part_rerank(), close_part_rerank();
extern int init_local_partvec(), local_partvec(), close_local_partvec();
static PROC_TAB proc_vec_vec[] = {
    "part_rerank",	init_part_rerank,part_rerank, close_part_rerank,
    "local_partvec",	init_local_partvec, local_partvec,
                        close_local_partvec,
    };
static int num_proc_vec_vec =
    sizeof (proc_vec_vec) / sizeof (proc_vec_vec[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to match a vector ctype against a vector ctype
 *1 local.retrieve.ctype_vec
 *2 sim_inner (vec_pair, sim_ptr, inst)
 *3   VEC_PAIR *vec_pair;
 *3   float *sim_ptr;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a vector pair and computes the similarity between
 *7 the two vectors, returning the value in sim_ptr.  Each vector is assumed
 *7 to be a single ctype.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "inner_match", "inner_partvec",
 *8 "inner_partvec_match".
***********************************************************************/
/* Run a ctype from vector against a ctype from a vector, returning a sim */
extern int init_sim_inner_match(), sim_inner_match(), close_sim_inner_match();
static PROC_TAB proc_ctype_vec[] = {
    "inner_match",init_sim_inner_match,sim_inner_match,	close_sim_inner_match,
    };
static int num_proc_ctype_vec =
    sizeof (proc_ctype_vec) / sizeof (proc_ctype_vec[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to match a vector ctype against a vector ctype
 *1 local.retrieve.partvecs
 *2 sim_inner_partvec (pvec_pair, pvec_results, inst)
 *3   PART_VEC_PAIR *pvec_pair;
 *3   PART_VEC_RESULTS *results;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a vector pair and computes the similarity between
 *7 the two vectors, returning the value in sim_ptr.  Each vector is assumed
 *7 to be a single ctype.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "inner_partvec", "inner_partvec_match".
***********************************************************************/
/* Run a ctype from part vector against a ctype from a part vector,
 * returning an array  similarities */
extern int init_sim_inner_partvec(), sim_inner_partvec(),
                     close_sim_inner_partvec();
extern int init_sim_inner_partvec_match(), sim_inner_partvec_match(),
                     close_sim_inner_partvec_match();
static PROC_TAB proc_partvecs[] = {
    "inner_partvec", init_sim_inner_partvec, sim_inner_partvec,
                     close_sim_inner_partvec,
    "inner_partvec_match", init_sim_inner_partvec_match,
                     sim_inner_partvec_match,
                     close_sim_inner_partvec_match,
    };
static int num_proc_partvecs =
    sizeof (proc_partvecs) / sizeof (proc_partvecs[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to match a vector list against a vector list
 *1 retrieve.vecs_vecs
 *2 * (vec_list_pair, results , inst)
 *3   VEC_LIST_PAIR *vec_list_pair;
 *3   ALT_RESULTS *results;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures take a vector list pair.
 *7 Each vector in vec_list_pair->vec_list1 is compared against every
 *7 vector in vec_list_pair->vec_list2.  All non-zero similarities are
 *7 returned in results.
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "vecs_vecs_cutoff", "partvec_partvec"
***********************************************************************/
/* Run a group of vectors against a group of vectors, returning all sims */
extern int init_vecs_vecs_cutoff(), vecs_vecs_cutoff(), close_vecs_vecs_cutoff();
extern int init_sim_partvecs(), sim_partvecs(), close_sim_partvecs();
static PROC_TAB proc_vecs_vecs[] = {
    "vecs_vecs_cutoff",  init_vecs_vecs_cutoff, vecs_vecs_cutoff,
                                                   close_vecs_vecs_cutoff,
    "partvec_partvec",	init_sim_partvecs, sim_partvecs,
                                           close_sim_partvecs,
    };
static int num_proc_vecs_vecs =
    sizeof (proc_vecs_vecs) / sizeof (proc_vecs_vecs[0]);



/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to maintain a list of top-ranked docs
 *1 local.retrieve.rank_tr
 *2 * (new, results, inst)
 *3   TOP_RESULT *new;
 *3   RESULT *results;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 Add the new document to results.top_results if the similarity is
 *7 high enough.  Break ties among similarities by procedure 
 *7 specific means.
 *7 Return UNDEF if error, 0 if not added, else 1.
 *8 Current possibilities are "rank_sdid".
***********************************************************************/
extern int rank_sdid();
static PROC_TAB proc_rank_tr[] = {
    "rank_sdid",	INIT_EMPTY,	rank_sdid,	CLOSE_EMPTY,
    };
static int num_proc_rank_tr = sizeof (proc_rank_tr) /
         sizeof (proc_rank_tr[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedure to assign sims to docs based on local sims
 *1 local.retrieve.local_comb
 *2 * (parts_results, results, inst)
 *3   TOP_RESULT *local_top_results;
 *3   RESULT *results;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 Combining global similarity (from results->top_results) and local 
 *7 similarity (eg paragraphs) from local_top_results of docs 
 *7 to query, assign final similarity to docs in results->top_results. 
 *7 Return UNDEF if error, 0 if not added, else 1.
 *8 Current possibilities are "req_save"
***********************************************************************/
extern int init_local_comb_req_save(), local_comb_req_save(), close_local_comb_req_save();
static PROC_TAB proc_local_comb[] = {
    "req_save",     init_local_comb_req_save, local_comb_req_save, 
                    close_local_comb_req_save,
    };
static int num_proc_local_comb = sizeof (proc_local_comb) /
         sizeof (proc_local_comb[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to get the next query for a retrieval
 *1 local.retrieve.get_query
 *2 * (unused, query_vec, inst)
 *3   char *unused;
 *3   QUERY_VECTOR *query_vec;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 Return the next query for a retrieval in query_vec.
 *7 Return UNDEF if error, 0 if no more queries, else 1.
 *8 Current possibilities are "get_q_random_vec",  "get_q_vec_file"

***********************************************************************/
extern int init_get_q_random_vec(), get_q_random_vec(),
    close_get_q_random_vec();
extern int init_get_q_vec_file(), get_q_vec_file(), close_get_q_vec_file();
static PROC_TAB proc_get_query[] = {
    "get_q_random_vec", init_get_q_random_vec,get_q_random_vec,
                                                        close_get_q_random_vec,
    "get_q_vec_file",init_get_q_vec_file,get_q_vec_file,close_get_q_vec_file,
    };
static int num_proc_get_query = sizeof (proc_get_query) /
         sizeof (proc_get_query[0]);

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local hierarchy table: procedures to output results of a retrieval
 *1 local.retrieve.output
 *2 * (results, tr_vec, inst)
 *3   RESULT *results;
 *3   TR_VEC *tr_vec;
 *3   int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 Take the results of a retrieval in results, and output in appropriate
 *7 form.  If possible, also add the top-ranked results to tr_vec
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "ret_regress"
***********************************************************************/
extern int init_ret_regress(), ret_regress(), close_ret_regress();
static PROC_TAB proc_r_output[] = {
    "ret_regress",      init_ret_regress,ret_regress,   close_ret_regress,
    };
static int num_proc_r_output = sizeof (proc_r_output) /
         sizeof (proc_r_output[0]);

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Local Hierarchy table: table of other hierarchy tables for retrieval
 *1 local.retrieve
 *7 Procedure table mapping a string procedure table name to either another
 *7 table of hierarchy tables, or to a stem table which maps
 *7 names to procedures
 *8 Current possibilities are "coll_sim",  "vec_vec", "ctype_vec", 
 *8 "rank_tr", "local_comb", "get_query", "output", "vecs_vecs".
***********************************************************************/
TAB_PROC_TAB lproc_retrieve[] = {
    "coll_sim",	TPT_PROC, NULL,	proc_coll_sim,    	&num_proc_coll_sim,
    "vec_vec",  TPT_PROC, NULL,	proc_vec_vec,  		&num_proc_vec_vec,
    "ctype_vec",TPT_PROC, NULL, proc_ctype_vec,		&num_proc_ctype_vec,
    "vecs_vecs",TPT_PROC, NULL, proc_vecs_vecs,         &num_proc_vecs_vecs,
    "rank_tr",	TPT_PROC, NULL,	proc_rank_tr,     	&num_proc_rank_tr,
    "local_comb",TPT_PROC, NULL,proc_local_comb,     	&num_proc_local_comb,
    "get_query",TPT_PROC, NULL,	proc_get_query,   	&num_proc_get_query,
    "output",	TPT_PROC, NULL,	proc_r_output,    	&num_proc_r_output,
    "partvecs",	TPT_PROC, NULL, proc_partvecs,		&num_proc_partvecs,
  };

int num_lproc_retrieve = sizeof (lproc_retrieve) / sizeof (lproc_retrieve[0]);
