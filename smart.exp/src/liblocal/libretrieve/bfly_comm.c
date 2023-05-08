#ifndef lint
/******************************************************************
 *
 * Routines which provide Butterfly inter-processor communication.
 *
 ******************************************************************/

#define SAY(x) PRINT_TRACE(2,print_string,x)
static char temp[100];

#include <us.h>
#include <stdio.h>
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
#include "inv.h"
#include "docdesc.h"

#define GetRemoteMem(num,typ,pnum) \
    ((typ *) UsAllocOnUsProc(pnum,num*sizeof(typ)))
#define GetLocalMem(num,typ) ((typ *) UsAllocLocal(num*sizeof(typ)))
#define ZapLocalMem(x,num,typ) (bzero( (char *) x, sizeof(typ)*num ))

/************* data for sending query to slaves *************/
typedef struct {
    int	ProcNum;
    short DoQuery_sem;
    short Terminate;

    VEC *Query;		/* the query itself */
    int Qmax_ctypes;	/* Query->ctypes allocated */
    int Qmax_conwts;	/* Query->conwts allocated */

    TR_VEC *PrevSeen;	/* the TR vector */
    int PSsize;		/* PrevSeen->tr allocated */

                        } QUERY_BLOCK;

static QUERY_BLOCK *slave_q;

static RETRIEVAL s_in_retrieval;	/* for building query at slave */
static QUERY_VECTOR s_query;

/************* data for receiving the results from a slave ********/

typedef struct {
    int index;			/* slave number responding */
    short *xfer_result;		/* alloc'd on slave */
    short result_xfered;	/* alloc'd on master */
            } RESULT_BLOCK;

static RESULT_BLOCK *slave_r;
static long *slave_r_count;

static RESULT *master_result;


/************ data used only in the slaves *************/

static QUERY_BLOCK *s_in;	/* our own query block */
static RESULT_BLOCK *s_out;	/* our own response block */
static int p;			/* processor (really index) */

/************ other data ***************/

static int inst = 0;
static int US_inited = 0;	/* has US_init() been called? */
static int *num_slaves;


static UsGenDesc *slave_genid;
static char *database;
static int num_wanted;
static PROC_INST rank_tr;
static PROC_INST coll_sim;

static void SlaveInit(), SlaveWork(), SlaveClose();
static int process_query();
static void clear_bfly_response_area(), bfly_wait_for_all_ready();
static int US_init(), US_close();
static void copy_input_to_slave(), free_query();
static void send_result_to_master();
static void copy_result_to_master();
static void signal_response_ready();
static RESULT *get_next_result();
static void print_query();

static SPEC_PARAM spec_args[] = {
    "database",			getspec_string,	(char *) &database,
    "distributed.coll_sim",	getspec_func,	(char *) &coll_sim.ptab,
    "retrieve.rank_tr",		getspec_func,	(char *) &rank_tr.ptab,
    "retrieve.num_wanted",      getspec_long,	(char *) &num_wanted,
    TRACE_PARAM ("retrieve.inverted.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


static SM_INDEX_DOCDESC doc_desc;

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Send a query out to all slave processors in distributed retrieval
 *1 bfly.retrieve.send_query
 *2 bfly_sendq( qvec, tr )
 *3 VEC *qvec
 *3 TR_VEC *tr
 *4 init_bfly_sendq( spec, max_docid )
 *5    "distributed.coll_sim"
 *5    "retrieve.rank_tr"
 *5    "retrieve.num_wanted"
 *4 close_bfly_sendq()

 *7 Wait for each slave processor to be ready to receive a query.
 *7 Then copy the query to its local memory and signal that it should
 *7 begin processing.
 *7 Returns 0 on success, UNDEF on failure.

 *9 This routine is for the butterfly only.

***********************************************************************/
int
init_bfly_sendq( spec, passed_max_docid )
SPEC *spec;
int passed_max_docid;
{
    long i;

    if ( inst != 0 ) {
	set_error( SM_ILLPA_ERR, "can't init twice", "init_bfly_sendq" );
	return (UNDEF);
    }
    inst++;

    if (US_init( spec, passed_max_docid ) == UNDEF)
	return UNDEF;

    PRINT_TRACE( 1, print_string, "Trace: entering init_bfly_sendq" );

    for (i=0; i<*num_slaves; i++) {	/* lock all semaphores */
	slave_q[i].DoQuery_sem = 0;
	UsLock( &slave_q[i].DoQuery_sem, 0 );
    }

    slave_genid = AsyncGenOnIFull( 0/*init*/, &SlaveWork, 0/*close*/,
				   spec, *num_slaves,
				   0/*maxprocs*/, 0/*not abortable*/ );

    PRINT_TRACE( 1, print_string, "Trace: leaving init_bfly_sendq" );
    return 0;
}

int
bfly_sendq( in_retrieval )
RETRIEVAL *in_retrieval;
{
    long i;

    PRINT_TRACE( 1, print_string, "Trace: entering bfly_sendq" );

    printf( "$$$ press return when ready to send to slaves\n" );
    (void) gets( temp );

    SAY( "*** - waiting for all slaves to be ready" );
    bfly_wait_for_all_ready();
    SAY( "*** - cleaning up response area" );
    clear_bfly_response_area();

/*    printf( "*** - query is as follows\n" );
 *    print_query( "***   > ", in_retrieval );
 */

    for (i=0; i<*num_slaves; i++) {
	sprintf( temp, "*** - sending query to slave idx %d", i );
	SAY( temp );
	copy_input_to_slave( in_retrieval, i );
	sprintf( temp, "*** - activating slave idx %d", i );
	SAY( temp );
	UsUnlock( &slave_q[i].DoQuery_sem );
    }

    PRINT_TRACE( 1, print_string, "Trace: leaving bfly_sendq" );
    return (0);
}

int
close_bfly_sendq()
{
    long i;

    PRINT_TRACE( 1, print_string, "Trace: entering close_bfly_sendq" );

    (void) US_close();

    inst--;
    PRINT_TRACE( 1, print_string, "Trace: leaving close_bfly_sendq" );
    return (0);
}


/********************   PROCEDURE DESCRIPTION   ************************
 *0 Gather retrieval results from slave processors in distributed retrieval
 *1 bfly.retrieve.get_results
 *2 bfly_get_res( results )
 *3 RESULT *results;
 *4 init_bfly_get_res( spec, passed_max_docid )
 *5    "retrieve.rank_tr"
 *5    "retrieve.num_wanted"
 *4 close_bfly_get_res()

 *7 Wait for a slave to return some results.  As each is returned,
 *7 merge them into one consolidated result and send that back.
 *7 Returns 0 on success, UNDEF otherwise.

 *9 For the butterfly only.

***********************************************************************/
int
init_bfly_get_res( spec, passed_max_docid )
SPEC *spec;
int passed_max_docid;
{
    SAY( "** in init_bfly_get_res" );
    if ( US_init( spec, passed_max_docid ) == UNDEF )
	return UNDEF;
    PRINT_TRACE( 1, print_string, "Trace: entering init_bfly_get_res" );

    PRINT_TRACE( 1, print_string, "Trace: leaving init_bfly_get_res" );
    return (0);
}

int
bfly_get_res( results )
RESULT *results;
{
    long i;
    RESULT *r;

    PRINT_TRACE( 1, print_string, "Trace: entering bfly_get_res" );

    for (i=0; i<*num_slaves; i++) {
	if ( NULL == ( r = get_next_result( i )))
	    return UNDEF;
	printf( "*** - received response #%d\n", i );

/*	merge slavereponse to result-response; */
    }

    PRINT_TRACE( 1, print_string, "Trace: leaving bfly_get_res" );
    return (0);
}

int
close_bfly_get_res()
{
    PRINT_TRACE( 1, print_string, "Trace: entering close_bfly_get_res" );

    (void) US_close();

    PRINT_TRACE( 1, print_string, "Trace: leaving close_bfly_get_res" );
    return (0);
}

/******************************************************************
 *
 * These are the slave processor routines.  We want to pass the
 * index into the init and close routines, so we can't take advantage
 * of the Uniform System's calling them.  Instead, we call them
 * by hand.  Sigh.
 *
 * Note that since we're now a slave process, we have a private copy
 * of the spec args.  We're going to modify the "database" parameter,
 * therefore, to point to our own inverted files.
 *
 ******************************************************************/
static void
SlaveInit( spec, idx )
SPEC *spec;
int idx;
{
    char *spec_database = "database";
    char spec_newdb[PATH_LEN];
    char *spec_new_specs[2];
    long i;
    int foo = 0;

    s_in->ProcNum = UsProc_Node;

    printf( "*%d* - entering slave init (pid %d)\n", p, getpid() );

    /* Change database to point to split inverted file */
    spec_new_specs[0] = spec_database;
    spec_new_specs[1] = spec_newdb;
    sprintf( spec_newdb, "%s/inv%03d/%03d",
	     database, *num_slaves, idx );
    printf( "*%d* - setting %s to `%s'\n", p, spec_new_specs[0],
	   spec_new_specs[1] );
    if ( UNDEF == mod_spec( spec, 2, spec_new_specs )) {
	printf( "*%d* - error modifying `database'\n", p );
	return;
    }
    printf( "*%d* - database has been changed\n", p );

    /* This is where queries will be received from the master. */
    s_in->Terminate = 0;

    s_in->Query = GetLocalMem( 1, VEC );
    s_in->Qmax_ctypes = 0;
    s_in->Qmax_conwts = 0;

    s_in->PrevSeen = GetLocalMem( 1, TR_VEC );
    s_in->PSsize = 0;

    s_out->xfer_result = GetLocalMem( 1, short );
    *s_out->xfer_result = 0;
    printf( "*%d* - xfer_result is at %d\n", p, (int) s_out->xfer_result );

    printf( "*%d* - getting ready to init coll_sim\n", p );
    if (UNDEF == (coll_sim.inst =
		  coll_sim.ptab->init_proc( spec, NULL ))) {
	printf( "*%d* - error init'ing coll_sim\n", p );
	return;
    }
    printf( "*%d* - done init'ing coll_sim\n", p );

    signal_response_ready( idx );
    printf( "*%d* - exiting slave init\n", p );
}


static void
SlaveWork( spec, idx )
SPEC *spec;
int idx;
{
    RESULT *results;
    RETRIEVAL in_retrieval;
    QUERY_VECTOR query;
    TR_VEC trvec;

    p = idx;	/* for all status messages */

    printf( "*%d* - present and accounted for (idx=%d)!\n", p, idx );

    s_in = &slave_q[ idx ];	/* set these up as shorthand */
    s_out = &slave_r[ idx ];
    printf( "*%d* - s_out is at %d\n", p, (int) s_out );

    SlaveInit( spec, idx );	/* call from here to get idx */

    printf( "*%d* - locking to request a query (lock=%d)\n", p,
	   (int) s_in->DoQuery_sem );
    UsLock( &s_in->DoQuery_sem, 0 );	/* ready for first query */
    while ( s_in->Terminate == 0 ) {
	printf( "*%d* - preparing to process query (lock=%d)\n", p,
	       (int) s_in->DoQuery_sem );
	in_retrieval.query = &query;
	query.vector = s_in->Query;
	in_retrieval.tr = &trvec;
	trvec.tr = s_in->PrevSeen;

/*	printf( "*%d* - received following query\n", p );
 *	sprintf( temp, "*%d*   > ", p );
 *	print_query( temp, &in_retrieval );
 */
	if (UNDEF == coll_sim.ptab->proc( &in_retrieval, results,
					  coll_sim.inst )) {
	    printf( "*%d* - error getting similarity\n", p );
	}
		       
	send_result_to_master( results, idx );
	
	printf( "*%d* - locking to request next query (lock=%d)\n", p,
	       (int) s_in->DoQuery_sem );
	UsLock( &s_in->DoQuery_sem, 0 );	/* next query? */
    }
    printf( "*%d* - received Terminate of 1; exiting\n", p );

    SlaveClose( idx );			/* call from here to get idx */
}


static void
SlaveClose( idx )
int idx;
{
    QUERY_BLOCK *s_in = &slave_q[ idx ];

    if (UNDEF == coll_sim.ptab->close_proc( coll_sim.inst )) {
	printf( "*%d* - error shutting down coll_sim\n", p );
	return;
    }

    UsFree( s_in->Query );
    UsFree( s_in->PrevSeen );

}


/******************************************************************
 *
 * Service routines.
 *
 ******************************************************************/
/*
 * Set up for a new round of response-ready's.
 */
static void
clear_bfly_response_area()
{
    long i;

    for (i=0; i<*num_slaves; i++) {
	slave_r[i].index = -1;
	slave_r[i].result_xfered = 0;
	if ( slave_r[i].xfer_result != NULL )
	    *slave_r[i].xfer_result = 0;
    }
    *slave_r_count = 0;
}    

/*
 * Wait for the next slave to signal its ready.  Then tell the
 * slave to start sending, wait for it to be finished, and return
 * the address of where the slave is supposed to put things.
 */
static RESULT
*get_next_result( cnt )
int cnt;
{
    int idx;

    while ( cnt>=*slave_r_count || slave_r[cnt].index==-1 )
	UsWait( 0 );

    printf( "*** - got response from index %d\n", slave_r[cnt].index );
    *slave_r[cnt].xfer_result = 1;
    while ( slave_r[cnt].result_xfered == 0 )
	UsWait( 0 );
    printf( "*** - response xfered from index %d\n", slave_r[cnt].index );

    return &master_result;

}

static void
signal_response_ready( idx )
int idx;
{
    long slot;

    slot = Atomic_add_long( slave_r_count, 1 );
    printf( "*%d* - got response slot %d\n", p, slot );
    slave_r[slot].index = idx;
}



/*
 * Get an ordering slot; wait for the master to request the 
 * results, copy them over, and then signal that we're done.
 */
static void
send_result_to_master( r, idx )
RESULT *r;
int idx;
{
    long slot;

    slot = Atomic_add_long( slave_r_count, 1 );
    printf( "*%d* - got response slot %d\n", p, slot );

    slave_r[slot].index = idx;
    printf( "*%d* - xfer result is %d @ %d\n", p,
	   (int) *(slave_r[slot].xfer_result),
	   (int) slave_r[slot].xfer_result );
    while ( *slave_r[slot].xfer_result == 0 )
	UsWait( 0 );
    printf( "*%d* - xfer result is %d @ %d\n", p,
	   (int) *slave_r[slot].xfer_result,
	   (int) slave_r[slot].xfer_result );

    printf( "*%d* - slot %d about to copy response to master\n",
	   p, slot );
    copy_result_to_master( r );
    slave_r[slot].result_xfered = 0;
}

/*
 * Wait for all processors to signal that they're ready.
 */
static void
bfly_wait_for_all_ready()
{
    printf( "*** - waiting for ready (%d / %d)\n", *slave_r_count,
	   *num_slaves );
    while ( *slave_r_count < *num_slaves )
	UsWait( 0 );
    while ( slave_r[ *num_slaves - 1 ].index == -1 )
	UsWait( 0 );
    printf( "*** - done waiting for ready (%d / %d)\n", *slave_r_count,
	   *num_slaves );
    printf( "*** - idx's are %d, %d, %d, %d\n",
	    slave_r[0].index,
	    slave_r[1].index,
	    slave_r[2].index,
	    slave_r[3].index );

}





/******************************************************************
 *
 * Make a copy of the query on processor at index i, and point
 * the slave's ->Query vector at the memory.
 *
 ******************************************************************/
static void
copy_input_to_slave( r, idx )
RETRIEVAL *r;
int idx;
{
    VEC *newvec = slave_q[idx].Query;
    int *max_ctype = &slave_q[idx].Qmax_ctypes;
    int *max_conwt = &slave_q[idx].Qmax_conwts;
    int proc = slave_q[idx].ProcNum;

    TR_VEC *newtr = slave_q[idx].PrevSeen;
    int *max_tr = &slave_q[idx].PSsize;

    VEC *qvec = (VEC *) r->query->vector;
    TR_VEC *tr = (TR_VEC *) r->tr;

    /* copy over the counts in the query vector */
    newvec->num_ctype = qvec->num_ctype;
    newvec->id_num = qvec->id_num;
    newvec->num_conwt = qvec->num_conwt;

    /* copy over the ctype-length fields */
    if ( newvec->num_ctype > *max_ctype ) {
	if ( *max_ctype != 0 )
	    UsFree( newvec->ctype_len );
	*max_ctype = newvec->num_ctype;
	newvec->ctype_len = GetRemoteMem( *max_ctype, long, proc );
    }
    bcopy( qvec->ctype_len, newvec->ctype_len,
	   newvec->num_ctype * sizeof(long));

    /* copy over the concept lists */
    if ( newvec->num_conwt > *max_conwt ) {
	if ( *max_conwt != 0 )
	    UsFree( newvec->con_wtp );
	*max_conwt = newvec->num_conwt;
	newvec->con_wtp = GetRemoteMem( *max_conwt, CON_WT, proc );
    }
    bcopy( qvec->con_wtp, newvec->con_wtp,
	  newvec->num_conwt * sizeof(CON_WT));

    /* copy over the previously-seen information */
    newtr->num_tr = tr->num_tr;
    if ( newtr->num_tr > *max_tr ) {
	if ( *max_tr != 0 )
	    UsFree( newtr->tr );
	*max_tr = newtr->num_tr;
	newtr->tr = GetRemoteMem( *max_tr, TR_TUP, proc );
    }
    bcopy( tr->tr, newtr->tr, newtr->num_tr * sizeof( TR_TUP ));

}

static void
free_query( qvec )
VEC *qvec;
{
    UsFree( qvec->ctype_len );
    UsFree( qvec->con_wtp );
    UsFree( qvec );
}



static void
print_query( prefix, r )
char *prefix;
RETRIEVAL *r;
{
    long i;
    QUERY_VECTOR *qv = r->query;
    TR_VEC *trvec = r->tr;
    VEC *qvec = (VEC *) qv->vector;
    long num_wt = qvec->num_conwt;
    CON_WT *wt = qvec->con_wtp;
    long num_clen = qvec->num_ctype;
    long *clen = qvec->ctype_len;

    int num_tr = trvec->num_tr;
    TR_TUP *tr = trvec->tr;

    printf( "%sctypelen=%d, numwt=%d, numtr=%d\n", prefix,
	    num_clen, num_wt, num_tr );
    printf( prefix );
    for (i=0; i<num_clen; i++)
	printf( "%d ", (int) clen[i] );
    printf( "\n" );

    printf( prefix );
    for (i=0; i<num_wt; i++)
	printf( "(%d,%f) ", (int) wt[i].con, wt[i].wt );
    printf( "\n" );

    printf( prefix );
    for (i=0; i<num_tr; i++)
	printf( "%d ", (int) tr[i].did );
    printf( "\n\n" );

}


/******************************************************************
 *
 * Copy the results from a local slave buffer to the master's
 * buffer (which is presumably on another node but in shared memory).
 *
 ******************************************************************/
static void
copy_result_to_master( r )
RESULT *r;
{
    TOP_RESULT *top = master_result->top_results;
    float *full = master_result->full_results;

    printf( "*%d* - copying %d top results\n", p, r->num_top_results );
    master_result->num_top_results = r->num_top_results;
    bcopy( r->top_results, top, r->num_top_results * sizeof(TOP_RESULT));

    printf( "*%d* - copying %d full results\n", p, r->num_full_results );
    master_result->num_full_results = r->num_full_results;
    bcopy( r->full_results, full, r->num_full_results * sizeof(float));
}	

static void
free_result( r )
RESULT *r;
{
    UsFree( r->top_results );
    r->num_top_results = 0;
    UsFree( r->full_results );
    r->num_full_results = 0;
}

/******************************************************************
 *
 * Initialize the Uniform System.  This routine is only needed
 * once, so a flag tells us whether or not to bother doing it
 * again.
 *
 ******************************************************************/
static int
US_init( spec, max_docid )
SPEC *spec;
int max_docid;
{
    long i;

    if ( US_inited )
	return 0;
    US_inited = 1;

    if ( UNDEF == lookup_spec( spec, &spec_args[0], num_spec_args ))
	return UNDEF;

    PRINT_TRACE( 1, print_string, "entering US_init" );

    if (UNDEF == lookup_spec_docdesc( spec, &doc_desc ))
	return UNDEF;

    InitializeUs();
    num_slaves = GetLocalMem( 1, int );
    *num_slaves = ProcsInUse() - 1;	/* don't count self */
    Share( &num_slaves );

    sprintf( temp, "** Running with %d slaves", *num_slaves );
    SAY( temp );

    if (NULL==(slave_q = GetLocalMem( *num_slaves, QUERY_BLOCK )))
	return (UNDEF);
    Share( &slave_q );
    sprintf( temp, "** Got slave_q at %d", (int) slave_q );
    SAY( temp );

    if (NULL==(slave_r = GetLocalMem( *num_slaves, RESULT_BLOCK )))
	return UNDEF;
    Share( &slave_r );
    for (i=0; i<*num_slaves; i++)
	slave_r[i].xfer_result = NULL;

    sprintf( temp, "** Got slave_r at %d", (int) slave_r );
    SAY( temp );

    if (NULL==(slave_r_count = GetLocalMem( 1, int )))
	return UNDEF;
    Share( &slave_r_count );
    sprintf( temp, "** Got slave_r_count at %d", (int) slave_r_count );
    SAY( temp );

    clear_bfly_response_area();

    /* Create a place for the slaves' results to be received */
    master_result = GetLocalMem( 1, RESULT );
    master_result->num_full_results = max_docid;
    master_result->full_results = GetLocalMem( max_docid+1, float );
    master_result->num_top_results = num_wanted;
    master_result->top_results = GetLocalMem( num_wanted+1, TOP_RESULT );
    Share( &master_result );
    sprintf( temp, "** Got master_result at %d", (int) master_result );
    SAY( temp );
    
    PRINT_TRACE( 1, print_string, "leaving US_init" );
}


static int
US_close()
{
    long i;

    if ( US_inited == 0 )	/* already closed */
	return 0;
    US_inited = 0;

    for (i=0; i<*num_slaves; i++) {
	slave_q[i].Terminate = 1;
	UsUnlock( &slave_q[i].DoQuery_sem );
    }

    WaitForTasksToFinish( slave_genid );

    free_result( master_result );
    UsFree( master_result );

    UsFree( slave_q );
    UsFree( slave_r );
    UsFree( slave_r_count );
    UsFree( num_slaves );

    TerminateUs();

    return 0;
}
#endif  /* lint */
