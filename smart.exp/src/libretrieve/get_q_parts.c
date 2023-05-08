#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/get_q_parts.c,v 11.0 1992/07/21 18:23:44 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Get next query from indexed query set, but re-parse breaking into parts.
 *1 retrieve.get_query.get_q_parts
 *2 get_q_parts (unused, query_vec, inst)
 *3   char *unused;
 *3   QUERY_VECTOR *query_vec;
 *3   int inst;

 *4 init_get_q_parts (spec, unused)
 *5   "query.textloc_file"
 *5   "query.textloc_file.rmode"
 *5   "index.query.preparse"
 *5   "retrieve.get_query.trace"
 *4 close_get_q_parts (inst)

 *6 global_context temporarily set to indicate indexing query
 *6 (CTXT_INDEXING_QUERY)

 *7 Get the text of the next query from an already indexed query set (query
 *7 given by query.textloc_file), and re-index it, breaking it into parts as
 *7 determined by the procedure given by preparse.  These parts are then
 *7 indexed by pp_veclist, yielding a vec_list which is returned in
 *7 query_vector.vector.

 *9 Warning: Make sure that the retrieval procedure specified is expecting
 *9 a vec_list instead of the more normal single vector.
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
#include "vector.h"
#include "docindex.h"
#include "textloc.h"


/******************************************************************
 *
 * Return the next query from a collection of documents, but return
 * it as a vector of queries.  The method of breaking the query into
 * parts is determined by the preparse choice.
 *
 ******************************************************************/

static PROC_INST preparser;       /* Preparse procedures */
static char *textloc_file;
static long textloc_mode;


static SPEC_PARAM spec_args[] = {
    "query.textloc_file",         getspec_dbfile,   (char *) &textloc_file,
    "query.textloc_file.rmode",   getspec_filemode, (char *) &textloc_mode,
    "index.query.preparse",    getspec_func,     (char *) &preparser.ptab,
    TRACE_PARAM ("retrieve.get_query.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static VEC_LIST vec_list;
static int textloc_fd;
static int pp_veclist_inst;

int
init_get_q_parts (spec, unused)
SPEC *spec;
char *unused;
{
    CONTEXT old_context;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_get_q_parts");

    /* Set context indicating that we are indexing a query.  Tells
       inferior procedures to use params appropriate for query instead of
       query */
    old_context = get_context();
    set_context (CTXT_INDEXING_QUERY);

    /* Open query textloc file for finding the queries */
    if (UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_mode)))
        return (UNDEF);

    /* Call all initialization procedures for indexing */
    if (UNDEF == (preparser.inst = preparser.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    if (UNDEF == (pp_veclist_inst = init_pp_veclist (spec, (char *) NULL)))
        return (UNDEF);

    vec_list.vec = NULL;
    vec_list.num_vec = 0;

    set_context( old_context );

    PRINT_TRACE (2, print_string, "Trace: leaving init_get_q_parts");
    return (0);
}

int
get_q_parts (unused, query_vec, inst)
char *unused;
QUERY_VECTOR *query_vec;
int inst;
{
    int status;
    SM_INDEX_TEXTDOC pp;
    TEXTLOC textloc;

    PRINT_TRACE (2, print_string, "Trace: entering get_q_parts");

    /* First dispose of any earlier vector we got */
    if ( vec_list.num_vec > 0 )
	if (UNDEF == free_vec_list( &vec_list ))
	    return (UNDEF);

    /* Get the query into memory */
    if (1 != (status = read_textloc (textloc_fd, &textloc)))
        return (status);

    /* Break it into little parts and put into a vector list */
    if (UNDEF == preparser.ptab->proc (&textloc, &pp, preparser.inst) ||
        UNDEF == pp_veclist (&pp, &vec_list, pp_veclist_inst) ||
	UNDEF == save_vec_list( &vec_list ))
	return (UNDEF);

    query_vec->qid = textloc.id_num;
    query_vec->vector = (char *) &vec_list;

    PRINT_TRACE (4, print_vec_list, &vec_list);
    PRINT_TRACE (2, print_string, "Trace: leaving get_q_parts");

    return (1);
}

int
close_get_q_parts (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_get_q_parts");

    if (UNDEF == preparser.ptab->close_proc (preparser.inst)||
        UNDEF == close_pp_veclist (pp_veclist_inst))
        return (UNDEF);

    if (UNDEF != textloc_fd && UNDEF == close_textloc (textloc_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_get_q_parts");
    return (0);
}


