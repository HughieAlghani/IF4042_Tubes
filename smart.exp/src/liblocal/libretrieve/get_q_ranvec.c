#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/get_q_ranvec.c,v 11.0 1992/07/21 18:22:30 chrisb Exp $";
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
#include "rr_vec.h"
#include "vector.h"

/* Return the next query from an ordinary vector collection */

static char *query_file;
static long query_file_mode;
static char *qrels_file;
static long qrels_file_mode;
static long how_many_queries;	      /* how many times to do this */

static SPEC_PARAM spec_args[] = {
    "retrieve.query_file",       getspec_dbfile, (char *) &query_file,
    "retrieve.query_file.rmode", getspec_filemode, (char *) &query_file_mode,
    "retrieve.qrels_file",       getspec_dbfile, (char *) &qrels_file,
    "retrieve.qrels_file.rmode", getspec_filemode, (char *) &qrels_file_mode,
    "retrieve.how_many_queries", getspec_long, (char *) &how_many_queries,
     TRACE_PARAM ("retrieve.get_query.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int query_fd = UNDEF;
static int qrels_fd = UNDEF;

/* static RR_VEC qrel_vec; */

static VEC vec;

int
init_get_q_random_vec (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_get_q_random_vec");

    if (! VALID_FILE (query_file)) {
        set_error (SM_ILLPA_ERR, "query_file", "init_get_q_random_vec");
        return (UNDEF);
    }

    if (UNDEF == (query_fd = open_vector (query_file, query_file_mode)))
        return (UNDEF);

    if (VALID_FILE (qrels_file) &&
        UNDEF == (qrels_fd = open_rr_vec (qrels_file, qrels_file_mode)))
        return (UNDEF);

    (void) srand(1);		/* constant seed to make testing easier */

    PRINT_TRACE (2, print_string, "Trace: leaving init_get_q_random_vec");
    return (0);
}

int
get_q_random_vec (unused1, query_vec, inst)
char *unused1;
QUERY_VECTOR *query_vec;
int inst;
{
    int status;

    PRINT_TRACE (2, print_string, "Trace: entering get_q_random_vec");

    if ( how_many_queries-- <= 0 )
	return (0);			/* all done */

    /*
     * Get a query that has at least one relevant document.
     */
    do {
	/* yes, this returns a number in the start..end range */
	/* bit shifting is to clean up the less random low-order bits */
	vec.id_num = global_start
	       + ((rand() >> 6) % (global_end - global_start + 1 ));

/*	     qrel_vec.qid = vec.id_num;
             if (1 != seek_rr_vec (qrels_fd, &qrel_vec))
                 vec.id_num = 0;

 */
    } while ( vec.id_num == 0 );
    
    if (UNDEF == seek_vector( query_fd, &vec ))
	return (UNDEF);
   if (1 == (status = read_vector (query_fd, &vec))) {
        query_vec->qid = vec.id_num;
        query_vec->vector = (char *) &vec;
    }

    PRINT_TRACE (4, print_vector, &vec);
    PRINT_TRACE (2, print_string, "Trace: leaving get_q_random_vec");
    return (status);
}

int
close_get_q_random_vec (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_get_q_random_vec");

    if (UNDEF == close_vector (query_fd))
        return (UNDEF);

    if (UNDEF != qrels_fd && UNDEF == close_rr_vec (qrels_fd))
        return (UNDEF);
    qrels_fd = UNDEF;

    PRINT_TRACE (2, print_string, "Trace: leaving close_get_q_random_vec");
    return (0);
}


