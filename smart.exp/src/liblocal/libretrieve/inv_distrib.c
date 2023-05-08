#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/inv_distrib.c,v 11.0 1992/07/21 18:22:31 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Do inverted file retrieval, with a distributed inverted file
 *1 bfly.retrieve.distributed
 *2 inv_distrib( in_retrieval, results, inst )
 *3 RETRIEVAL *in_retrieval;
 *3 RESULT *results;
 *3 int inst;
 *4 init_inv_distrib( spec, unused )
 *5    "distributed.send_query"
 *5    "distributed.get_results"
 *5    "retrieve.rank_tr"
 *5    "retrieve.num_wanted"
 *5    "retrieve.doc_file"
 *5    "retrieve.doc.textloc_file"
 *4 close_inv_distrib( inst )

 *7 Use the "send query" routine to send the query out to all slave
 *7 processors.  Initialize a return area (for top and full results),
 *7 and then use the "get results" routine to retrieve the results
 *7 from the various processors.
 *7 Returns 0 on success; UNDEF otherwise.

 *9 Designed for the Butterfly and probably works only there (though
 *9 the hope is that it'll work elsewhere).

***********************************************************************/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "retrieve.h"
#include "vector.h"
#include "tr_vec.h"
#include "docdesc.h"

static char temp[100];

static PROC_INST rank_tr;
static PROC_INST distribute_query;
static PROC_INST gather_results;

static long num_wanted;

static char *doc_file;           /* Used only to tell how many docs */
static char *textloc_file;          /* in collection */

static SPEC_PARAM spec_args[] = {
    "distributed.send_query",	getspec_func,	(char *)&distribute_query.ptab,
    "distributed.get_results",	getspec_func,	(char *) &gather_results.ptab,
    "retrieve.rank_tr",       	getspec_func,	(char *) &rank_tr.ptab,
    "retrieve.num_wanted",      getspec_long,	(char *) &num_wanted,
    "retrieve.doc_file",        getspec_dbfile,	(char *) &doc_file,
    "retrieve.doc.textloc_file",   getspec_dbfile,	(char *) &textloc_file,
    TRACE_PARAM ("retrieve.inverted.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


static SM_INDEX_DOCDESC doc_desc;
static TOP_RESULT *top_results;
static float *full_results;
static int maxdocid;

int
init_inv_distrib( spec, unused )
SPEC *spec;
char *unused;
{
    REL_HEADER *rh;

    if (UNDEF == lookup_spec( spec, &spec_args[0], num_spec_args ))
	return (UNDEF);

    PRINT_TRACE( 1, print_string, "Trace: entering init_inv_distrib" );

#define SAY(x) PRINT_TRACE( 2, print_string, x )
    SAY( "** About to lookup doc desc" );
    if (UNDEF == lookup_spec_docdesc( spec, &doc_desc ))
	return (UNDEF);

    SAY( "** Trying to get max doc" );
    /* Find out the largest document number. */
    if ((NULL != (rh = get_rel_header (doc_file)) && rh->max_primary_value)||
        (NULL != (rh = get_rel_header (textloc_file)) && rh->max_primary_value))
        maxdocid = rh->max_primary_value;
    else
        maxdocid = MAX_DID_DEFAULT;

    (void) sprintf( temp, "** Max doc is %d", maxdocid );
    SAY( temp );

    /*
     * Init all of our procedures.  Note that RESULTS much be init'd
     * before DISTRIBUTE.
     */
    if (UNDEF==(gather_results.inst =
		         gather_results.ptab->init_proc(spec, maxdocid ) )
	|| UNDEF==(distribute_query.inst =
		         distribute_query.ptab->init_proc(spec,maxdocid ))
	|| UNDEF==(rank_tr.inst = rank_tr.ptab->init_proc(spec,NULL)))
	return (UNDEF);
	
    SAY( "** Init'd subroutines of inv_distrib" );

    /* Reserve space for top ranked documents */
    if (NULL == (top_results = (TOP_RESULT *) malloc ((unsigned) (num_wanted+1)
                                                      * sizeof (TOP_RESULT))))
        return (UNDEF);
    if (NULL == (full_results = (float *) 
                 malloc ((unsigned) (maxdocid + 1) * sizeof (float)))) {
        set_error (errno, "", "inverted");
        return (UNDEF);
    }

    PRINT_TRACE( 1, print_string, "Trace: leaving init_inv_distrib" );
    return (0);
}


int
inv_distrib( in_retrieval, results, inst )
RETRIEVAL *in_retrieval;
RESULT *results;
int inst;
{
    long i;

    QUERY_VECTOR *query_vec = in_retrieval->query;
    VEC *qvec = (VEC *) (query_vec->vector);
    
    PRINT_TRACE( 1, print_string, "Trace: entering inv_distrib" );

    if (UNDEF == distribute_query.ptab->proc( qvec, in_retrieval->tr ))
	return (UNDEF);

    results->qid = query_vec->qid;
    results->top_results = top_results;
    results->num_top_results = num_wanted;
    results->full_results = full_results;
    results->num_full_results = maxdocid + 1;

    /* Ensure all values in top_results are 0 initially */
    bzero ((char *) top_results, 
           sizeof (TOP_RESULT) * (int) (num_wanted + 1));
    bzero ((char *) full_results, 
           sizeof (float) * (int) (maxdocid+1));

    /* Handle docs that have been previously seen by setting sim */
    /* to large negative number */
    for (i = 0; i < in_retrieval->tr->num_tr; i++)
        full_results[in_retrieval->tr->tr[i].did] = -1.0e8;


    if (UNDEF == gather_results.ptab->proc( results ))
	return (UNDEF);

    PRINT_TRACE( 1, print_string, "Trace: leaving inv_distrib" );
    return (1);
}


int
close_inv_distrib( inst )
int inst;
{
    PRINT_TRACE( 1, print_string, "Trace: entering close_inv_distrib" );

    if (UNDEF==(rank_tr.ptab->close_proc(rank_tr.inst))
	|| UNDEF==(distribute_query.ptab->close_proc(distribute_query.inst) )
	|| UNDEF==(gather_results.ptab->close_proc(gather_results.inst) ))
	return (UNDEF);
	
    (void) free( (char *) top_results );

    PRINT_TRACE( 1, print_string, "Trace: leaving close_inv_distrib" );
    return (0);
}
