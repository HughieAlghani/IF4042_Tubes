#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/ret_random.c,v 11.0 1992/07/21 18:22:28 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/******************************************************************
 *
 * Modified from ret_tr.
 * Selects and displays information about a random element in the
 * results.  Output is intended to be used in regression analysis.
 *
 ******************************************************************/
#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "io.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "retrieve.h"
#include "tr_vec.h"
#include "rr_vec.h"
#include "vector.h"
#include "docindex.h"
#include "textloc.h"
#include "context.h"

static char *qrels_file;
static long qrels_file_mode;

static PROC_INST para_pre;
static PROC_INST sent_pre;
static PROC_INST sect_retrieve;      /* Retrieval method for sections */
static char *textloc_file;
static long textloc_file_mode;

static char *weight;

static char *regress_outfile;	/* where to put our results */
static FILE *outfd;		/* and it's fd */

static SPEC_PARAM spec_args[] = {
    "retrieve.qrels_file",       getspec_dbfile, (char *) &qrels_file,
    "retrieve.qrels_file.rmode", getspec_filemode, (char *) &qrels_file_mode,
    "retrieve.ret_random.regress_file", getspec_string,
                                            (char *) &regress_outfile,
    "inter.doc.textloc_file",       getspec_dbfile,  (char *) &textloc_file,
    "inter.doc.textloc_file.rmode", getspec_filemode,(char *) &textloc_file_mode,
    "retrieve.ret_random.para.preparse", getspec_func, (char *) &para_pre.ptab,
    "retrieve.ret_random.sent.preparse", getspec_func, (char *) &sent_pre.ptab,
    "retrieve.ret_random.vecs_vecs", getspec_func,(char *)&sect_retrieve.ptab,
    "sect_weight",		    getspec_string, (char *) &weight,
    TRACE_PARAM ("retrieve.output.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int qrels_fd = UNDEF;

static RR_VEC qrels;

static TR_TUP *next_tup();

static int textloc_fd;
static int pp_veclist_inst;

static int get_section_vector();
static int normalize_vector_weights();

int
init_ret_random (spec, unused)
SPEC *spec;
char *unused;
{
    CONTEXT old_context;	/* temporary savings place */

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_ret_random");

    /* we require unnormalized weights; we'll also normalize them later */
    if ( weight[2] != 'n' ) {
	set_error( SM_ILLPA_ERR, "sect_weight must be ??n", "init_ret_random");
	return (UNDEF);
    }

    /* Open qrels file, if desired */
    if (VALID_FILE (qrels_file) &&
        UNDEF == (qrels_fd = open_rr_vec (qrels_file, qrels_file_mode)))
        return (UNDEF);

    if ( regress_outfile == NULL || *regress_outfile == '\0' ) {
        set_error (SM_ILLPA_ERR, "regress_file", "init_ret_random");
        return (UNDEF);
    }

    /* Open Textloc file to get file location and section offsets for did */
    if (UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_file_mode)))
        return (UNDEF);

    /* For making a vector of the sections returned from preparsing */
    if (UNDEF==(pp_veclist_inst=init_pp_veclist(spec, (char *)NULL)))
	return (UNDEF);

    /* init the paragraph preparsing routine */
    old_context = get_context();
    set_context( CTXT_DOC );
    if (UNDEF == (para_pre.inst = para_pre.ptab->init_proc (spec, NULL)))
	return(UNDEF);
    if (UNDEF == (sent_pre.inst = sent_pre.ptab->init_proc (spec, NULL)))
	return(UNDEF);
    set_context( old_context );

    if ( *regress_outfile == '-' )
	outfd = stdout;
    else if ( NULL == (outfd = fopen( regress_outfile, "w" ))) {
	set_error(SM_ILLPA_ERR, "can't open regress_file", "init_ret_random");
	return (UNDEF);
    }

    (void) fprintf( outfd, "%5s %5s %8s %3s %8s %8s %8s %8s %8s %8s %3s\n",
	   "Query", "Docid", "CosSim", "Rnk",
	    "MaxPSim", "AvgPSim",
	    "MaxPnSim", "AvgPnSim",
	    "MaxSSim", "AvgSSim",
	    "Rel" );

    PRINT_TRACE (2, print_string, "Trace: leaving init_ret_random");
    return (0);
}


/******************************************************************
 *
 * Select a random element in the results and textloc its
 * information.
 *
 ******************************************************************/
int
ret_random (results, tr_vec, inst)
RESULT *results;
TR_VEC *tr_vec;
int inst;
{
    register RR_TUP *qr_ptr, *end_qr_ptr;
    int num_retrieved;
    int next_iter;
    int docnum, i, relevant;
    float psim_max, psim_sum, psim_avg;		/* paragraph vals */
    float pnsim_max, pnsim_sum, pnsim_avg;	/* normalized vals */
    float ssim_max, ssim_sum, ssim_avg;		/* sentence vals */
    register TR_TUP *tr_tup;
    TOP_RESULT *r;
    VEC_LIST d1vec, d2vec;
    VEC_LIST_PAIR vecpair;
    ALT_RESULTS sim_results;

    PRINT_TRACE (2, print_string, "Trace: entering ret_random");

    if (results == NULL || tr_vec == NULL) 
        return (UNDEF);

    if (qrels_fd != UNDEF) {
        qrels.qid = results->qid;
        if (UNDEF == seek_rr_vec (qrels_fd, &qrels) ||
            UNDEF == read_rr_vec (qrels_fd, &qrels))
            return (UNDEF);
        end_qr_ptr = &qrels.rr[qrels.num_rr];
    }

    tr_vec->qid = results->qid;

    next_iter = 0;
    for (i = 0; i < tr_vec->num_tr; i++) {
        /* Set next_iter to indicate */
        /* current top docs came from new iteration of feedback */
        if (next_iter <= tr_vec->tr[i].iter) {
            next_iter = tr_vec->tr[i].iter + 1;
        }
    }
    
    /*
     * Find out how many top ranked documents were actually retrieved (the
     * num_top_results item just tells how big the vector is).
     */
    num_retrieved = results->num_top_results;
    while ( results->top_results[num_retrieved - 1].sim == 0.0 && num_retrieved>0 )
	num_retrieved--;
    if ( num_retrieved == 0 )
	return (UNDEF);

    /*
     * random number from 0 to num-1, which choses the document.
     * Loop until we don't have the document as a query (okay if only one match).
     */
    do {
	docnum = (rand() >> 6) % num_retrieved;
	r = &(results->top_results[docnum]);
    } while ( num_retrieved > 1 && r->did == results->qid );

    relevant = 0;
    if (qrels_fd != UNDEF) {
	/* Get relevance judgement for this did */
	for (qr_ptr = &qrels.rr[0];
	     qr_ptr < end_qr_ptr && qr_ptr->did < r->did;
	     qr_ptr++)
	    ;
	if (qr_ptr < end_qr_ptr && qr_ptr->did == r->did)
	    relevant = 1;
    }


    /*
     * Get the two documents and their respective sets of paragraphs.
     */
    if (UNDEF==get_section_vector( r->did, &para_pre, &d1vec )
	|| UNDEF == save_vec_list( &d1vec ))
	return (UNDEF);
    if (UNDEF==get_section_vector( results->qid, &para_pre, &d2vec )
	|| UNDEF == save_vec_list( &d2vec ))
	return (UNDEF);
    
    /*
     * Get the paragraph-paragraph similarities and loop through
     * to get the info we want.  This is unnormalized weights.
     */
    vecpair.vec_list1 = &d1vec;
    vecpair.vec_list2 = &d2vec;
    if (UNDEF == sect_retrieve.ptab->proc(&vecpair,
					  &sim_results,
					  sect_retrieve.inst))
	return (UNDEF);

    psim_max = psim_sum = 0.0;
    for (i=0; i<sim_results.num_results; i++) {
	psim_sum += sim_results.results[i].sim;
	if ( sim_results.results[i].sim > psim_max )
	    psim_max = sim_results.results[i].sim;
    }
    if ( sim_results.num_results == 0 )
	psim_avg = 0.0;
    else
	psim_avg = psim_sum / (float) sim_results.num_results;

    /*
     * Now we do the paragraphs with NORMALIZED vector weights.
     */
    if (UNDEF==normalize_vector_weights( &d1vec )
	|| UNDEF==normalize_vector_weights( &d2vec ))
	return (UNDEF);	
    
    /*
     * Paragraph/paragraph similarities NORMALIZED.
     */
    vecpair.vec_list1 = &d1vec;
    vecpair.vec_list2 = &d2vec;
    if (UNDEF == sect_retrieve.ptab->proc(&vecpair,
					  &sim_results,
					  sect_retrieve.inst))
	return (UNDEF);

    pnsim_max = pnsim_sum = 0.0;
    for (i=0; i<sim_results.num_results; i++) {
	pnsim_sum += sim_results.results[i].sim;
	if ( sim_results.results[i].sim > pnsim_max )
	    pnsim_max = sim_results.results[i].sim;
    }
    if ( sim_results.num_results == 0 )
	pnsim_avg = 0.0;
    else
	pnsim_avg = pnsim_sum / (float) sim_results.num_results;


    if (UNDEF == free_vec_list( &d1vec )
	|| UNDEF == free_vec_list( &d2vec ))
	return (UNDEF);			/* replace unused storage */

    /*
     * Now get the documents' set of sentences.
     */
    if (UNDEF==get_section_vector( r->did, &sent_pre, &d1vec )
	|| UNDEF == save_vec_list( &d1vec ))
	return (UNDEF);
    if (UNDEF==get_section_vector( results->qid, &sent_pre, &d2vec )
	|| UNDEF == save_vec_list( &d2vec ))
	return (UNDEF);

    
    /*
     * Get the sentence-sentence similarities and loop through
     * to get the info we want.
     */
    vecpair.vec_list1 = &d1vec;
    vecpair.vec_list2 = &d2vec;
    if (UNDEF == sect_retrieve.ptab->proc(&vecpair,
					  &sim_results,
					  sect_retrieve.inst))
	return (UNDEF);

    ssim_max = ssim_sum = 0.0;
    for (i=0; i<sim_results.num_results; i++) {
	ssim_sum += sim_results.results[i].sim;
	if ( sim_results.results[i].sim > ssim_max )
	    ssim_max = sim_results.results[i].sim;
    }
    if ( sim_results.num_results == 0 )
	ssim_avg = 0.0;
    else
	ssim_avg = ssim_sum / (float) sim_results.num_results;

    if (UNDEF == free_vec_list( &d1vec )
	|| UNDEF == free_vec_list( &d2vec ))
	return (UNDEF);			/* replace unused storage */



    /*
     * Seems like this routine expects to get something back, so...
     */
    if (NULL == (tr_tup = next_tup (tr_vec)))
	return (UNDEF);
    tr_tup->sim = r->sim;
    tr_tup->did = r->did;
    tr_tup->rank = 1;
    tr_tup->iter = next_iter;
    tr_tup->action = '\0';
    tr_tup->rel = relevant;

    (void) fprintf( outfd,
	    "%5d %5d %8.6f %3d %8.4f %8.4f %8.6f %8.6f %8.4f %8.4f %3d\n",
	    results->qid,
	    r->did,
	    r->sim,
	    docnum+1,
	    psim_max, psim_avg,
	    pnsim_max, pnsim_avg,
	    ssim_max, ssim_avg,
	    relevant );

    PRINT_TRACE (2, print_string, "Trace: leaving ret_random");
    return (1);
}


/******************************************************************
 *
 * Shut 'er down.
 *
 ******************************************************************/
int
close_ret_random (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_ret_random");

    if ( outfd != stdout )
	(void) fclose( outfd );

    if (UNDEF != qrels_fd && UNDEF == close_rr_vec (qrels_fd))
        return (UNDEF);

    if (UNDEF == close_pp_veclist( pp_veclist_inst )
        || UNDEF == para_pre.ptab->close_proc (para_pre.inst))
	return (UNDEF);

    outfd = NULL;
    qrels_fd = UNDEF;

    PRINT_TRACE (2, print_string, "Trace: leaving close_ret_random");
    return (0);
}


static TR_TUP *tr_tup_buf;
static int size_tr_tup_buf = 0;

/* Return address of next spot in tr_vec->tr that can be filled.  If
   need more space, then allocate. */
static TR_TUP *
next_tup (tr_vec)
TR_VEC *tr_vec;
{
    TR_TUP *old_tr_tup_buf;

    if (tr_vec->tr == tr_tup_buf && tr_vec->num_tr < size_tr_tup_buf)
        return (&tr_vec->tr[tr_vec->num_tr++]);

    old_tr_tup_buf = tr_tup_buf;

    /* Reserve more space */
    size_tr_tup_buf += tr_vec->num_tr + 15;
    if (NULL == (tr_tup_buf = (TR_TUP *) malloc ((unsigned) size_tr_tup_buf *
                                                 sizeof (TR_TUP))))
        return (NULL);

    /* Copy existing list into new space */
    if (tr_vec->num_tr > 0)
        bcopy ((char *) tr_vec->tr,
               (char *) tr_tup_buf,
               (int) tr_vec->num_tr * sizeof (TR_TUP));

    /* Free old space if it was allocated here */
    if (tr_vec->tr == old_tr_tup_buf && tr_vec->num_tr > 0)
        (void) free ((char *) old_tr_tup_buf);

    tr_vec->tr = tr_tup_buf;

    return (&tr_vec->tr[tr_vec->num_tr++]);
}

/******************************************************************
 *
 * Given a document number, retrieve that document and get a
 * vector containing all of its paragraphs.
 *
 ******************************************************************/
static int
get_section_vector( did, preparser, vec )
long did;
PROC_INST *preparser;
VEC_LIST *vec;
{
    SM_INDEX_TEXTDOC pp;
    TEXTLOC textloc;

    textloc.id_num = did;

   if (1 != seek_textloc (textloc_fd, &textloc) ||
        1 != read_textloc (textloc_fd, &textloc))
        return (UNDEF);

    if (UNDEF == preparser->ptab->proc (&textloc, &pp, preparser->inst))
        return (UNDEF);

    if (UNDEF == pp_veclist( &pp, vec, pp_veclist_inst ))
	return (UNDEF);

    return (1);
}


/******************************************************************
 *
 * Zip through each vector and apply the cosine normalization to
 * it so we'll have normalized weights.
 *
 ******************************************************************/
static int
normalize_vector_weights( veclist )
VEC_LIST *veclist;
{
    long i;

    for (i=0; i < veclist->num_vec; i++) {
/*	if (UNDEF==norm_proc->proc( veclist->vec[i].con_wtp,  */
/*			       veclist->vec[i].num_conwt ))   */
        if (UNDEF == normwt_cos (&veclist->vec[i], (char *) NULL, 0))
	    return (UNDEF);
    }

    return (1);
}
