#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/ret_regress.c,v 11.0 1992/07/21 18:22:28 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/* If defined, the random document is chosen from the top documents
 * rather than from the full set (see pick_random()).
 */
/*#define RANDOM_FROM_TOP */

/* If defined, relevant documents (when the docid is specifically
 * chosen) are discarded unless they lie in the top results.
 */
/*#define OMIT_NONTOP_RELS */

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Display stats about query relative to specified document number.
 *1 retrieve.output.ret_regress
 *2 ret_regress(results, tr_vec, inst)
 *3 RESULT *results;
 *3 TR_VEC *tr_vec;
 *3 int inst;
 *4 init_ret_regress( spec, unused )
 *4 close_ret_regress( inst )
 *5    "retrieve.get_query.get_doc"
 *5    "retrieve.qrels_file"
 *5    "retrieve.qrels_file.rmode"
 *5    "retrieve.ret_regress.regress_file"
 *5    "doc.textloc_file"
 *5    "doc.textloc_file.rmode"
 *5    "retrieve.parts.sect.preparse"
 *5    "retrieve.parts.para.preparse"
 *5    "retrieve.parts.sent.preparse"
 *5    "retrieve.parts.sent.thresh"
 *5    "retrieve.vecs_vecs"
 *5    "sect_weight"
 *5    "retrieve.output.trace"
 
 *7 Compare the query to the document specified by the routine
 *7 retrieve.get_query.get_doc.  The qrels file is used to determine
 *7 whether or not the document is relevant to the query.
 *7
 *7 Next break both the query and the document into paragraphs using
 *7 para.preparse and do a pairwise comparison of the resulting vectors
 *7 using vecs_vecs.  Do the same, then, for the paragraphs with
 *7 normalized weights.  Then do the same for all sentences in the
 *7 query and document, using sent.preparse.  Note that this routine
 *7 requires that sect_weight have a third character of 'n' to indicate
 *7 that section weights are normally unnormalized.  And do the
 *7 same thing for the sections of the document.
 *7
 *7 Finally, printout the various useful statistics gathered by this
 *7 routine. The intent is that the output can be used in regression
 *7 analysis to determine the relative merit of the various stats.

 *8 Implemented for building lists for regression analysis.  Probably
 *8 not overly useful elsewhere.

***********************************************************************/

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

static PROC_INST get_docnum;
static PROC_INST sect_pre;
static PROC_INST para_pre;
static PROC_INST sent_pre;
static PROC_INST vecs_vecs;      /* Pairwise sim of vector lists */
static char *textloc_file;
static long textloc_file_mode;

static long ssim_thresh;	/* threshold for sent sim count */
static char *weight;

static char *regress_outfile;	/* where to put our results */
static FILE *outfd;		/* and it's fd */

static SPEC_PARAM spec_args[] = {
    "retrieve.get_query.get_doc",getspec_func, (char *) &get_docnum.ptab,
    "retrieve.qrels_file",       getspec_dbfile, (char *) &qrels_file,
    "retrieve.qrels_file.rmode", getspec_filemode, (char *) &qrels_file_mode,
    "retrieve.ret_regress.regress_file", getspec_string,
                                            (char *) &regress_outfile,
    "doc.textloc_file",       	    getspec_dbfile,  (char *) &textloc_file,
    "doc.textloc_file.rmode",       getspec_filemode,
                                            (char *) &textloc_file_mode,
    "retrieve.parts.sect.preparse", getspec_func, (char *) &sect_pre.ptab,
    "retrieve.parts.para.preparse", getspec_func, (char *) &para_pre.ptab,
    "retrieve.parts.sent.preparse", getspec_func, (char *) &sent_pre.ptab,
    "retrieve.parts.sent.thresh",   getspec_long, (char *) &ssim_thresh,
    "retrieve.vecs_vecs",	    getspec_func,(char *)&vecs_vecs.ptab,
    "sect_weight",		    getspec_string, (char *) &weight,
    TRACE_PARAM ("retrieve.output.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int qrels_fd = UNDEF;
static int logit = 0;	/* for watching what happens */

static RR_VEC qrels;

static TR_TUP *next_tup();

static int textloc_fd;
static int pp_veclist_inst;

ALT_RESULTS sim_results;

static int get_section_vector();
static int normalize_vector_weights();
static long pick_random();

int
init_ret_regress (spec, unused)
SPEC *spec;
char *unused;
{
    CONTEXT old_context;	/* temporary savings place */

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_ret_regress");

    /* we require unnormalized weights; we'll also normalize them later */
    if ( weight[2] != 'n' ) {
	set_error( SM_ILLPA_ERR, "sect_weight must be ??n", "init_ret_regress");
	return (UNDEF);
    }

    /* Open qrels file, if desired */
    if (VALID_FILE (qrels_file) &&
        UNDEF == (qrels_fd = open_rr_vec (qrels_file, qrels_file_mode)))
        return (UNDEF);

    if ( regress_outfile == NULL || *regress_outfile == '\0' ) {
        set_error (SM_ILLPA_ERR, "regress_file", "init_ret_regress");
        return (UNDEF);
    }

    /* Open Textloc file to get file location and section offsets for did */
    if (UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_file_mode)))
        return (UNDEF);

    if (UNDEF==(get_docnum.inst = get_docnum.ptab->init_proc(spec,NULL)))
	return (UNDEF);

    /* For making a vector of the sections returned from preparsing */
    if (UNDEF==(pp_veclist_inst=init_pp_veclist(spec, (char *)NULL)))
	return (UNDEF);

    /* For pairwise comparisons of sets of vectors. */
    if (UNDEF == (vecs_vecs.inst = vecs_vecs.ptab->init_proc( spec, NULL)))
	return (UNDEF);

    /* init the parts preparsing routine */
    old_context = get_context();
    set_context( CTXT_DOC );
    if (UNDEF == (sect_pre.inst = sect_pre.ptab->init_proc (spec, NULL)))
	return(UNDEF);
    if (UNDEF == (para_pre.inst = para_pre.ptab->init_proc (spec, NULL)))
	return(UNDEF);
    if (UNDEF == (sent_pre.inst = sent_pre.ptab->init_proc (spec, NULL)))
	return(UNDEF);
    set_context( old_context );

    if ( *regress_outfile == '-' )
	outfd = stdout;
    else if ( NULL == (outfd = fopen( regress_outfile, "w" ))) {
	set_error(SM_ILLPA_ERR, "can't open regress_file", "init_ret_regress");
	return (UNDEF);
    }

    (void) fprintf( outfd, "%5s %5s %6s %6s %8s %8s %8s %8s %8s %3s %3s\n",
		   "Query", "Docid", "Qlen", "Dlen", "Sim",
		   "MaxCnSim", "AvgCnSim",
		   "MaxPnSim", "AvgPnSim",
		   "Snt",
		   "Rel" );

    PRINT_TRACE (2, print_string, "Trace: leaving init_ret_regress");
    return (0);
}


/******************************************************************
 *
 * Select a random element in the results and textloc its
 * information.
 *
 ******************************************************************/
int
ret_regress (results, tr_vec, inst)
RESULT *results;
TR_VEC *tr_vec;
int inst;
{
    RR_TUP *qr_ptr;
    RR_TUP *end_qr_ptr = NULL;
    int next_iter;
    long docnum;
    float docsim;
    long i, relevant;
    float csim_max, csim_sum, csim_avg;		/* section vals */
#ifdef UNNORMALIZED
    float psim_max, psim_sum, psim_avg;		/* paragraph vals */
#endif
    float pnsim_max, pnsim_sum, pnsim_avg;	/* normalized vals */
    float ssim_max;				/* sentence vals */
    int ssim_over_thresh;			/* sents over thresh */
    TR_TUP *tr_tup;
    VEC_LIST d1vec, d2vec;
    long d1len, d2len;
    VEC_LIST_PAIR vecpair;

    PRINT_TRACE (2, print_string, "Trace: entering ret_regress");

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
     * Get our document and isolate its doc-doc similarity.  If
     * no docnum was explictly given, choose one closest in match
     * to the query.
     */
    docnum = get_docnum.ptab->proc( get_docnum.inst );
    if (docnum == 0) {
	if (-1 == (docnum = pick_random( results )))
	    return (UNDEF);
    }
#ifdef OMIT_NONTOP_RELS
    else {	
	short okay = 0; /* okay only if relevant is in the top results */

	for (i=0; i<results->num_top_results; i++ )
	    if ( results->top_results[i].did == docnum )
		okay=1;
	if (okay != 1)
	    goto end_point; /* yeah, yeah, yeah */
    }
#endif
    docsim = results->full_results[docnum];

    relevant = 0;
    if (qrels_fd != UNDEF) {
	/* Get relevance judgement for this did */
	for (qr_ptr = &qrels.rr[0];
	     qr_ptr < end_qr_ptr && qr_ptr->did < docnum;
	     qr_ptr++)
	    ;
	if (qr_ptr < end_qr_ptr && qr_ptr->did == docnum)
	    relevant = 1;
    }

    /*
     * Get the sets of sections for the two documents.  Then do
     * a pairwise comparison of the sets.  The get the information
     * that we want.
     */
    if (UNDEF==get_section_vector( docnum, &sect_pre, &d1vec, &d1len )
	|| UNDEF == save_vec_list( &d1vec ))
	return (UNDEF);
    if (UNDEF==get_section_vector( results->qid, &sect_pre, &d2vec, &d2len )
	|| UNDEF == save_vec_list( &d2vec ))
	return (UNDEF);

    /* We'll use cosine-normalized section weights. */
    if (UNDEF==normalize_vector_weights( &d1vec )
	|| UNDEF==normalize_vector_weights( &d2vec ))
	return (UNDEF);	

    vecpair.vec_list1 = &d1vec;
    vecpair.vec_list2 = &d2vec;
    if (UNDEF == vecs_vecs.ptab->proc(&vecpair,
					  &sim_results,
					  vecs_vecs.inst))
	return (UNDEF);

    csim_max = csim_sum = 0.0;
    for (i=0; i<sim_results.num_results; i++) {
	csim_sum += sim_results.results[i].sim;
	if ( sim_results.results[i].sim > csim_max )
	    csim_max = sim_results.results[i].sim;
    }
    if ( sim_results.num_results == 0 )
	csim_avg = 0.0;
    else
	csim_avg = csim_sum / (float) sim_results.num_results;

    if (UNDEF == free_vec_list( &d1vec )
	|| UNDEF == free_vec_list( &d2vec ))
	return (UNDEF);			/* replace unused storage */


    /*
     * Get the two documents and their respective sets of paragraphs.
     */
    if (UNDEF==get_section_vector( docnum, &para_pre, &d1vec, (long *)NULL )
	|| UNDEF == save_vec_list( &d1vec ))
	return (UNDEF);
    if (UNDEF==get_section_vector( results->qid, &para_pre, &d2vec,
				  (long *)NULL )
	|| UNDEF == save_vec_list( &d2vec ))
	return (UNDEF);
    
    /*
     * Get the paragraph-paragraph similarities and loop through
     * to get the info we want.  This is unnormalized weights.
     */
#ifdef UNNORMALIZED
    vecpair.vec_list1 = &d1vec;
    vecpair.vec_list2 = &d2vec;
    if (UNDEF == vecs_vecs.ptab->proc(&vecpair,
					  &sim_results,
					  vecs_vecs.inst))
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

#endif
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
    if (UNDEF == vecs_vecs.ptab->proc(&vecpair,
					  &sim_results,
					  vecs_vecs.inst))
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
    if (UNDEF==get_section_vector( docnum, &sent_pre, &d1vec,
				   (long *) NULL )
	|| UNDEF == save_vec_list( &d1vec ))
	return (UNDEF);
    if (UNDEF==get_section_vector( results->qid, &sent_pre, &d2vec,
				   (long *) NULL )
	|| UNDEF == save_vec_list( &d2vec ))
	return (UNDEF);

    
    /*
     * Get the sentence-sentence similarities and loop through
     * to get the info we want.
     */
    vecpair.vec_list1 = &d1vec;
    vecpair.vec_list2 = &d2vec;
    if (UNDEF == vecs_vecs.ptab->proc(&vecpair,
					  &sim_results,
					  vecs_vecs.inst))
	return (UNDEF);

    ssim_max = 0.0;
    for (i=0; i<sim_results.num_results; i++) {
	if ( sim_results.results[i].sim > ssim_max )
	    ssim_max = sim_results.results[i].sim;
    }

    ssim_over_thresh = 0;
    for (i=0; i<sim_results.num_results; i++) {
	if ( sim_results.results[i].sim > ssim_thresh )
	    ssim_over_thresh++;
    }

    if (UNDEF == free_vec_list( &d1vec )
	|| UNDEF == free_vec_list( &d2vec ))
	return (UNDEF);			/* replace unused storage */



    (void) fprintf( outfd,
	    "%5d %5d %6d %6d %8.6f %8.6f %8.6f %8.6f %8.6f %3d %3d\n",
		   results->qid,
		   docnum,
		   d2len,
		   d1len,
		   docsim,
		   csim_max, csim_avg,
		   pnsim_max, pnsim_avg,
		   ssim_over_thresh,
		   relevant );
    if ( outfd != stdout && ++logit > 10 ) {
	(void) printf( "Just finished %d with %d\n", results->qid, docnum );
	logit = 0;
    }

    /*
     * Seems like this routine expects to get something back, so...
     */
#ifdef OMIT_NONTOP_RELS
  end_point:
#endif
    if (NULL == (tr_tup = next_tup (tr_vec)))
	return (UNDEF);
    tr_tup->sim = docnum;
    tr_tup->did = docsim;
    tr_tup->rank = 1;
    tr_tup->iter = next_iter;
    tr_tup->action = '\0';
    tr_tup->rel = relevant;

    PRINT_TRACE (2, print_string, "Trace: leaving ret_regress");
    return (1);
}


/******************************************************************
 *
 * Shut 'er down.
 *
 ******************************************************************/
int
close_ret_regress (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_ret_regress");

    if ( outfd != stdout )
	(void) fclose( outfd );

    if (UNDEF != qrels_fd && UNDEF == close_rr_vec (qrels_fd))
        return (UNDEF);

    if (UNDEF == close_pp_veclist( pp_veclist_inst )
        || UNDEF == para_pre.ptab->close_proc (sect_pre.inst)
        || UNDEF == para_pre.ptab->close_proc (para_pre.inst)
        || UNDEF == para_pre.ptab->close_proc (sent_pre.inst)
	|| UNDEF == vecs_vecs.ptab->close_proc (vecs_vecs.inst)
	|| UNDEF == get_docnum.ptab->close_proc( get_docnum.inst ))
	return (UNDEF);

    outfd = NULL;
    qrels_fd = UNDEF;

    PRINT_TRACE (2, print_string, "Trace: leaving close_ret_regress");
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
 * vector containing all of its paragraphs.  If length is passed
 * as non-NULL, the documents raw length is returned.
 *
 ******************************************************************/
static SM_INDEX_TEXTDOC pp;  /* so we don't leak the memory */
static TEXTLOC textloc;

static int
get_section_vector( did, preparser, vec, length )
long did;
PROC_INST *preparser;
VEC_LIST *vec;
long *length;
{
    textloc.id_num = did;

   if (1 != seek_textloc (textloc_fd, &textloc) ||
        1 != read_textloc (textloc_fd, &textloc))
        return (UNDEF);

    if (length != NULL)
	*length = textloc.end_text - textloc.begin_text;

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


/******************************************************************
 *
 * Choose a document at random from the top results passed.  We'd
 * prefer to get a non-relevant document, actually, so we'll make a
 * few passes trying to get a non-relevant document and then give
 * up.
 *
 ******************************************************************/
static long
pick_random( results )
RESULT *results;
{
    int num_retrieved;
    int seq, try_count, i;
    int rel = 0;
    long docnum;

#ifdef RANDOM_FROM_TOP
    num_retrieved = results->num_top_results;
    while ( results->top_results[num_retrieved-1].sim==0.0 && num_retrieved>0)
        num_retrieved--;
    if ( num_retrieved == 0 )
        return -1;
#else
    num_retrieved = results->num_full_results;
#endif
    /*
     * random number from 0 to num-1, which choses the document.
     * Loop until we don't have the document as a query
     * (okay if only 1 match)
     */
    try_count = 0;
    do {
        seq = (rand() >> 6) % num_retrieved;
#ifdef RANDOM_FROM_TOP
        docnum = results->top_results[seq].did;
#else
        docnum = seq;
#endif
	if (num_retrieved<=1)	/* If only one choice, scram */
	    break;

	if ( docnum == results->qid )	/* don't want to match itself */
	    continue;
	try_count++;

	rel = 0;
	for (i=0; i<qrels.num_rr; i++) {
	    if (qrels.rr[i].did == docnum)
		rel = 1;
	    if (qrels.rr[i].did >= docnum)
		break;
	}

    } while ( try_count < 20 && rel==1 );   /* try 20 times for non-rel */

    return (docnum);
}
