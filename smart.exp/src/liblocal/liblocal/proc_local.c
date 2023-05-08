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
 *0 Hierarchy table giving top level locally-used procedures.
 *1 local.top
 *2 * (unused1, unused2, inst)
 *3    char *unused1;
 *3    char *unused2;
 *3    int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures are top-level procedures used locally.
 *7
 *7 As top-level procedures, they are responsible for setting
 *7 trace conditions, and for determining other execution time constraints,
 *7 such as when execution should stop (eg, if global_end is exceeded).
 *7 Return UNDEF if error, else 1.
 *8 Current possibilities are "retrieve", "retrieve_all", "ret_fdbk" 
***********************************************************************/

extern int init_inter_web(), inter_web(), close_inter_web();

static PROC_TAB proc_toplocal[] = {
    "inter_web",    init_inter_web,  inter_web,    close_inter_web,
};
int num_proc_toplocal =
    sizeof (proc_toplocal) / sizeof (proc_toplocal[0]);



/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to match a vector ctype against a vector ctype
 *1 local.ctype_vec
 *2 sim_inner_match (vec_pair, sim_ptr, inst)
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
 *8 Current possibilities are "inner_match"
***********************************************************************/
/* Run a ctype from vector against a ctype from a vector, returning a sim */
extern int init_sim_inner_match(), sim_inner_match(), close_sim_inner_match();
static PROC_TAB proc_ctype_vec[] = {
    "inner_match", init_sim_inner_match,sim_inner_match, close_sim_inner_match,
    };
static int num_proc_ctype_vec =
    sizeof (proc_ctype_vec) / sizeof (proc_ctype_vec[0]);




/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table giving local/global similarity combining routines
 *1 local.local_comb
 *2 * (local_top_results, results, inst)
 *3    TOP_RESULT *local_top_results;
 *3    RESULT *results;
 *3    int inst;
 *4 init_* (spec, unused)
 *4 close_* (inst)
 *7 Procedure table mapping a string procedure name to procedure addresses.
 *7 These procedures are intended to combine local and global similarity
 *7 results.  This is an extension of the retrieve.local_comb hiearchy.
 *7
 *8 Current possibilities are "req_save"
***********************************************************************/

extern int init_local_comb_req_save(), local_comb_req_save(),
           close_local_comb_req_save();

static PROC_TAB proc_comb_local[] = {
    "req_save", init_local_comb_req_save, local_comb_req_save,
                             close_local_comb_req_save,
};
int num_proc_comb_local =
    sizeof (proc_comb_local) / sizeof (proc_comb_local[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: procedures to match a query against a collection
 *1 local.coll_sim
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
 *8 Current possibilities are "local_inv2"
**********************************************************************/

extern int init_local_inv2(), local_inv2(), close_local_inv2();

static PROC_TAB proc_coll_sim[] = {
    "local_inv",        init_local_inv2,     local_inv2,     close_local_inv2,
};
static int num_proc_coll_sim = sizeof (proc_coll_sim)/sizeof(proc_coll_sim[0]);


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Hierarchy table: table of other locally-used hierarchy tables
 *1 local
 *7 Procedure table mapping a string procedure table name to either another
 *7 table of hierarchy tables, or to a stem table which maps
 *7 names to procedures
 *8 Current possibilities are "top", "combine", "ctype_vec"
***********************************************************************/
TAB_PROC_TAB proc_local[] = {
    "top",	TPT_PROC, NULL,	proc_toplocal,	&num_proc_toplocal,
    "combine",	TPT_PROC, NULL, proc_comb_local, &num_proc_comb_local,
    "ctype_vec",TPT_PROC, NULL, proc_ctype_vec, &num_proc_ctype_vec,
    "coll_sim", TPT_PROC, NULL, proc_coll_sim,  &num_proc_coll_sim
  };

int num_proc_local = sizeof (proc_local) / sizeof (proc_local[0]);
