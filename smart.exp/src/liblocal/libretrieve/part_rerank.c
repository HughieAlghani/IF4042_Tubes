/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Adjust vector similarity based upon regression formula
 *1 retrieve.vec_vec.part_rerank
 *2 part_rerank (vec_pair, sim, inst)
 *3   VEC_PAIR *vec_pair;
 *3   float *sim;
 *3   int inst;
 *4 init_part_rerank (spec, unused)
 *5   "retrieve.regress.sect.preparse"
 *5   "retrieve.regress.para.preparse"
 *5   "retrieve.regress.sent.preparse"
 *5   "retrieve.regress.sent.thresh"
 *5   "retrieve.vecs_vecs"
 *5   "doc.textloc_file"
 *5   "doc.textloc_file.rmode"
 *5   "retrieve.req.rerank.trace"
 *5    "regress.coeff.constant"
 *5    "regress.coeff.avgcnsim"
 *5    "regress.coeff.maxpnsim"
 *5    "regress.coeff.lnmaxpsim"
 *5    "regress.coeff.avgpnsim"
 *5    "regress.coeff.lnmaxpnsim"
 *5    "regress.coeff.lnsents_over"
 *5    "regress.coeff.sumlnlen"
 *4 close_part_rerank (inst)
 *6 global_context temporarily set to indexing doc (CTXT_DOC)

 *7 Given the two vectors of vec_pair, and an incoming similarity between
 *7 the two, recalculate the similarity using a formula (presumably
 *7 derived from a regression analysis).  The formula depends upon looking
 *7 at the documents broken down into sentences, paragraphs, and
 *7 sections, calculating various relationships between those (maximum
 *7 section-section similarity, e.g.), and combining the results to give
 *7 a new similarity which predicts relevance.
 *7
 *7 The text corresponding to the two vectors (assumed to be both
 *7 documents), given by textloc_file, is preparsed by the procedures
 *7 corresponding to xxx.preparse.  Preparse will break the text of the
 *7 document up into appropriate sections, eg sentences.  The procedure
 *7 pp_veclist then constructs a vector for each section.  The pairs of
 *7 vec_list's (lists of vectors for each document) are then pairwise
 *7 compared using the procedure given by vecs_vecs, with all non-zero
 *7 similarities between vectors being reported.  
 *7 
 *7 Currently coefficient values:
 *7	constant	just added as is
 *7	avgcnsim	Avg section/section similarity, cosine norm
 *7	maxpnsim	Max para/para similarity, cosine norm
 *7	lnmaxpsim	Log of max para/para sim, no normalization
 *7	avgpnsim	Avg para/para sim, cosine norm
 *7	lnmaxpnsim	Log of maxpnsim
 *7	lnsents_over	Log of (1+number of sent pairs which exceed thresh)
 *7	sumlnlen	Log(Query len) + Log( Doc len)
 *7 After values are all summed together, the result is 
 *7 See req_vec_inv for a sample of a routine that would call part_rerank.

 *9 This is used in current experiments for regression analysis related
 *9 to doc-doc linking, and may not be overly useful elsewhere.
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
#include "textloc.h"
#include "docindex.h"

static PROC_INST sect_pre, para_pre, sent_pre;
static PROC_INST parts_ret;
static char *textloc_file;
static long textloc_mode;
static long ssim_thresh;      /* sent/sent similarity threshold */

#define LN(x) ((float) log( (double) (x)))
static float coeff_constant;	/* coefficients for the values that */
static float coeff_avgcnsim;	/* are used in this routine */
static float coeff_maxpnsim;
static float coeff_lnmaxpsim;
static float coeff_avgpnsim;
static float coeff_lnmaxpnsim;
static float coeff_lnsents_over;
static float coeff_sumlnlen;
/* WARNING: When adding new coefficients, ensure settings for NEED[]
 * are properly adjusted in the init routine
 */

static SPEC_PARAM spec_args[] = {
    "retrieve.parts.sect.preparse",   getspec_func,  (char *) &sect_pre.ptab,
    "retrieve.parts.para.preparse",   getspec_func,  (char *) &para_pre.ptab,
    "retrieve.parts.sent.preparse",   getspec_func,  (char *) &sent_pre.ptab,
    "retrieve.regress.sent.thresh",   getspec_long,  (char *) &ssim_thresh,
    "retrieve.vecs_vecs",        getspec_func,  (char *) &parts_ret.ptab,
    "doc.textloc_file",             getspec_dbfile,(char *) &textloc_file,
    "doc.textloc_file.rmode",       getspec_filemode, (char *) &textloc_mode,

    /* regress constant */
    "regress.coeff.constant",	getspec_float, (char *) &coeff_constant,

    /* values from section-section comparisons */
    "regress.coeff.avgcnsim",	getspec_float, (char *) &coeff_avgcnsim,

    /* values from paragraph-paragraph comparisons */
    "regress.coeff.maxpnsim",	getspec_float, (char *) &coeff_maxpnsim,
    "regress.coeff.lnmaxpsim",	getspec_float, (char *) &coeff_lnmaxpsim,
    "regress.coeff.avgpnsim",	getspec_float, (char *) &coeff_avgpnsim,
    "regress.coeff.lnmaxpnsim",	getspec_float, (char *) &coeff_lnmaxpnsim,

    /* values from sentence-sentence comparisons */
    "regress.coeff.lnsents_over",getspec_float, (char *) &coeff_lnsents_over,

    /* values for doc/query length comparisons */
    "regress.coeff.sumlnlen",	getspec_float, (char *) &coeff_sumlnlen,

    TRACE_PARAM ("retrieve.req.rerank.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int pp_veclist_inst;
static int textloc_fd;

static int num_inst = 0;

static int get_vec_lists();
static long QueryLen, DocLen;
static void get_sim_avg_max();
static int normalize_vector_weights();

/*
 * A VEC_LIST for each of sections, paras, and sentences, and one
 * of each of those for each of the two documents passed in.  This
 * is a "cache" of the most recently used.
 */
#define SECT 0
#define PARA 1
#define SENT 2
#define NUM_PARTS 3
static short NEED[NUM_PARTS];

static VEC_LIST vec_list[2][NUM_PARTS]
            = { {{NULL,0,0}, {NULL,0,0}, {NULL,0,0}},
		{{NULL,0,0}, {NULL,0,0}, {NULL,0,0}} };
static ALT_RESULTS sims[NUM_PARTS] = { {0,NULL}, {0,NULL}, {0,NULL} };

int
init_part_rerank (spec, unused)
SPEC *spec;
char *unused;
{
    CONTEXT old_context;
    long i,j;

    if (num_inst++ > 0)
        return (num_inst);

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_part_rerank");

    old_context = get_context();
    set_context (CTXT_DOC);
    if (UNDEF == (sect_pre.inst = sect_pre.ptab->init_proc (spec, NULL)) ||
	UNDEF == (para_pre.inst = para_pre.ptab->init_proc (spec, NULL)) ||
	UNDEF == (sent_pre.inst = sent_pre.ptab->init_proc (spec, NULL)) ||
        UNDEF == (parts_ret.inst = parts_ret.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    if (UNDEF == (pp_veclist_inst = init_pp_veclist (spec, (char *) NULL)))
        return (UNDEF);
    set_context (old_context);

    /* Open Textloc file to get file location and section offsets for did */
    if (UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_mode)))
        return (UNDEF);

    /* Initialize cached vec_lists */
    for (i=0; i<2; i++)
	for (j=0; j<NUM_PARTS; j++)
	    vec_list[i][j].id_num = UNDEF;

    NEED[SENT] = ( coeff_lnsents_over != 0.0 );
    NEED[PARA] = ( coeff_maxpnsim != 0.0 ||
		   coeff_lnmaxpsim != 0.0 ||
		   coeff_avgpnsim != 0.0 ||
		   coeff_lnmaxpnsim != 0.0 );
    NEED[SECT] = ( coeff_avgcnsim != 0.0 );

    PRINT_TRACE (2, print_string, "Trace: leaving init_part_rerank");
    return (num_inst);
}

int
part_rerank (vec_pair, sim, inst)
VEC_PAIR *vec_pair;
float *sim;
int inst;
{
    long i;
    float valu;
    VEC_LIST_PAIR vec_list_pair;
    float MaxPnSim, MaxPSim, AvgPnSim, AvgPSim;
    float MaxCnSim, AvgCnSim;
    int ssim_over_thresh = 0;
    

    PRINT_TRACE (2, print_string, "Trace: entering part_rerank");
    PRINT_TRACE (6, print_long, &vec_pair->vec1->id_num);
    PRINT_TRACE (6, print_long, &vec_pair->vec2->id_num);

    /* Get the first vector from the cache or afresh */
    if (vec_pair->vec1->id_num != vec_list[0][SECT].id_num) {
	for (i=0; i<NUM_PARTS; i++)
	    if (vec_list[0][i].id_num != UNDEF)
		(void) free_vec_list (&vec_list[0][i]);
        if (UNDEF == get_vec_lists (vec_pair->vec1->id_num,
				    &vec_list[0][SECT],
				    &vec_list[0][PARA],
				    &vec_list[0][SENT],
				    &QueryLen ))
            return (UNDEF);
    }

    /* Get the second vector from the cache or afresh */
    if (vec_pair->vec2->id_num != vec_list[1][SECT].id_num) {
	for (i=0; i<NUM_PARTS; i++)
	    if (vec_list[1][i].id_num != UNDEF)
		(void) free_vec_list (&vec_list[1][i]);

        if (UNDEF == get_vec_lists (vec_pair->vec2->id_num,
				    &vec_list[1][SECT],
				    &vec_list[1][PARA],
				    &vec_list[1][SENT],
				    &DocLen))
            return (UNDEF);
    }

    /* Do pairwise similarities */
    for (i=0; i<NUM_PARTS; i++) {
	if ( NEED[i] ) {
	    vec_list_pair.vec_list1 = &vec_list[0][i];
	    vec_list_pair.vec_list2 = &vec_list[1][i];

	    if (UNDEF == parts_ret.ptab->proc( &vec_list_pair,
					      &sims[i],
					      parts_ret.inst ))
		return (UNDEF);
	}
    }

    if ( NEED[PARA] ) {
	get_sim_avg_max( &sims[PARA], &MaxPSim, &AvgPSim );

	/* Now get the normalized paragraph similarity */
	if (UNDEF==normalize_vector_weights( &vec_list[0][PARA] ) ||
	    UNDEF==normalize_vector_weights( &vec_list[1][PARA] ))
	    return (UNDEF);

	vec_list_pair.vec_list1 = &vec_list[0][PARA];
	vec_list_pair.vec_list2 = &vec_list[1][PARA];
	if (UNDEF==parts_ret.ptab->proc( &vec_list_pair,
					&sims[PARA],
					&parts_ret.inst ))
	    return (UNDEF);
	get_sim_avg_max( &sims[PARA], &MaxPnSim, &AvgPnSim );
    }

    if ( NEED[SECT] ) {
	/* Now get the normalized section similarity */
	if (UNDEF==normalize_vector_weights( &vec_list[0][SECT] ) ||
	    UNDEF==normalize_vector_weights( &vec_list[1][SECT] ))
	    return (UNDEF);

	vec_list_pair.vec_list1 = &vec_list[0][SECT];
	vec_list_pair.vec_list2 = &vec_list[1][SECT];
	if (UNDEF==parts_ret.ptab->proc( &vec_list_pair,
					&sims[SECT],
					&parts_ret.inst ))
	    return (UNDEF);
	get_sim_avg_max( &sims[SECT], &MaxCnSim, &AvgCnSim );
    }

    if ( NEED[SENT] ) {
	/* Count sentence pairs with a similarity over the threshold */
	ssim_over_thresh = 0;
	for (i=0; i<sims[SENT].num_results; i++) {
	    if ( sims[SENT].results[i].sim > ssim_thresh )
		ssim_over_thresh++;
	}
    }

    valu = coeff_constant
         + coeff_avgcnsim     * AvgCnSim
	 + coeff_maxpnsim     * MaxPnSim
         + coeff_lnmaxpsim    * LN( MaxPSim )
	 + coeff_avgpnsim     * AvgPnSim
	 + coeff_lnmaxpnsim   * LN( MaxPnSim )
	 + coeff_lnsents_over * LN( ssim_over_thresh+1 )
	 + coeff_sumlnlen     * ( LN(QueryLen) + LN(DocLen) );

    *sim = (float) (exp( (double)valu ) / ( 1.0 + exp( (double)valu )));

    PRINT_TRACE (2, print_string, "Trace: leaving part_rerank");
    return (1);
}


int
close_part_rerank (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_part_rerank");

    if (--num_inst) {
        PRINT_TRACE (2, print_string, "Trace: leaving close_part_rerank");
        return (0);
    }

    if (UNDEF == sect_pre.ptab->close_proc (sect_pre.inst) ||
	UNDEF == para_pre.ptab->close_proc (para_pre.inst) ||
	UNDEF == sect_pre.ptab->close_proc (sent_pre.inst) ||
        UNDEF == parts_ret.ptab->close_proc (parts_ret.inst) ||
        UNDEF == close_pp_veclist (pp_veclist_inst) ||
        UNDEF == close_textloc (textloc_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_part_rerank");
    return (1);
}



/*
 * Given a document number, return the vectors of sections, paragraphs,
 * and sentences associated with it.
 */
TEXTLOC textloc;
SM_INDEX_TEXTDOC pp;

static int
get_vec_lists (did, sect_vl, para_vl, sent_vl, doclen)
long did;
VEC_LIST *sect_vl;
VEC_LIST *para_vl;
VEC_LIST *sent_vl;
long *doclen;
{

    textloc.id_num = did;
    pp.id_num = did;
    if (1 != seek_textloc (textloc_fd, &textloc) ||
        1 != read_textloc (textloc_fd, &textloc))
	return (UNDEF);

    if ( doclen != NULL )
	*doclen = textloc.end_text - textloc.begin_text;

    /* Get section vectors, then para, then sentence */
    if ( NEED[SECT] ) {
	if (UNDEF == sect_pre.ptab->proc (&textloc, &pp, sect_pre.inst) ||
	    UNDEF ==  pp_veclist (&pp, sect_vl, pp_veclist_inst) ||
	    UNDEF == save_vec_list (sect_vl))
	    return (UNDEF);
    }

    if ( NEED[PARA] ) {
	if (UNDEF == para_pre.ptab->proc (&textloc, &pp, para_pre.inst) ||
	    UNDEF ==  pp_veclist (&pp, para_vl, pp_veclist_inst) ||
	    UNDEF == save_vec_list (para_vl))
	    return (UNDEF);
    }

    if ( NEED[SENT] ) {
	if (UNDEF == para_pre.ptab->proc (&textloc, &pp, sent_pre.inst) ||
	    UNDEF ==  pp_veclist (&pp, sent_vl, pp_veclist_inst) ||
	    UNDEF == save_vec_list (sent_vl))
	    return (UNDEF);
    }

    return (1);
}



/******************************************************************
 *
 * Return the max and average similarities in a set of results.
 *
 ******************************************************************/
static void
get_sim_avg_max( sims, max, avg )
ALT_RESULTS *sims;
float *max;
float *avg;
{
    float sum = 0.0;
    long i;

    *max = 0.0;
    for (i=0; i<sims->num_results; i++) {
	sum += sims->results[i].sim;
	if ( sims->results[i].sim > *max )
	    *max = sims->results[i].sim;
    }

    if ( avg != NULL )
	*avg = sum / (float) sims->num_results;
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
/*      if (UNDEF==norm_proc->proc( veclist->vec[i].con_wtp,  */
/*                             veclist->vec[i].num_conwt ))   */
        if (UNDEF == normwt_cos (&veclist->vec[i], (char *) NULL, 0))
            return (UNDEF);
    }

    return (1);
}

