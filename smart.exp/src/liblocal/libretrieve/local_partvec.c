#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libretrieve/local_partvec.c,v 11.0 1992/07/21 18:22:33 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Calculate a sim between vectors based on sim between vector partvecs
 *1 retrieve.vec_vec.local_partvec
 *2 local_partvec (vec_pair, sim, inst)
 *3   VEC_PAIR *vec_pair;
 *3   float *sim;
 *3   int inst;
 *4 init_local_partvec (spec, unused)
 *5   "retrieve.parts.partvec_file"
 *5   "retrieve.parts.partvec_file.rmode"
 *5
 *5   "doc.textloc_file"		Used if partvec_file invalid
 *5   "doc.textloc_file.rmode"
 *5   "retrieve.parts.preparse"
 *5   "retrieve.parts.get_partvec"
 *5
 *5   "retrieve.partvecs_vecs"
 *5
 *5   "retrieve.parts.thresh"
 *5   "retrieve.parts.num_match"
 *5   "retrieve.local.vec_vec.trace"
 *4 close_local_partvec (inst)
 *6 global_context temporarily set to indexing doc (CTXT_DOC)

 *7 Given the two vectors of vec_pair, set sim to max similarity of
 *7 a part of vector1 to a part of vector2, assuming that there are
 *7 sufficient (num_match) parts of vector1 and vector2 which have 
 *7 similarity greater than thresh. Normally used as a restrictive function. 
 *7 For example, perform a normal retrieval between vectors, but then only
 *7 show user those documents whose sentences were sufficiently similar.
 *7 
 *7 The part vectors for the two passed vectors are retrieved from the
 *7 partvec_file if it is valid; otherwise, the documents are retrieved
 *7 via the textloc_file and parsed into part_vec's by pp_partvec.
 *7
 *7 In either case, the partvecs_vecs routine is then used to generate
 *7 a pairwise comparison of the parts of the two docs.
 *7 The number of pairs with
 *7 similarity greater than parts.thresh is then counted and compared with
 *7 parts.num_match. If greater, then sim is set to the maximum similarity,
 *7 else it is set to 0.0.
 *7 
 *7 See local_inv for a sample of a routine that would call local_partvec.

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
#include "part_vector.h"
#include "textloc.h"
#include "docindex.h"

static char *partvec_file;	  /* The easy way to get a part_vec */
static long partvec_mode;

static char *textloc_file;	  /* The brute-force way to get a part */
static long textloc_mode;	  /* vec, by building it */
static PROC_INST preparse;
static PROC_INST make_pvec;

static PROC_INST parts_compare;	  /* To compare two part vecs */

static float parts_thresh;        /* Threshold for a match between parts */
static long parts_match;	  /* Number that must match */

static SPEC_PARAM spec_args[] = {
    "retrieve.parts.partvec_file", getspec_dbfile, (char *)&partvec_file,
    "retrieve.parts.partvec_file.rmode", getspec_filemode,
                                                   (char *)&partvec_mode,

    "doc.textloc_file",          getspec_dbfile,(char *) &textloc_file,
    "doc.textloc_file.rmode",    getspec_filemode, (char *) &textloc_mode,
    "retrieve.parts.preparse",	 getspec_func, (char *) &preparse.ptab,
    "retrieve.parts.get_partvec", getspec_func, (char *) &make_pvec.ptab,

    "retrieve.partvecs_vecs",    getspec_func,  (char *) &parts_compare.ptab,

    "retrieve.parts.thresh",     getspec_float, (char *) &parts_thresh,
    "retrieve.parts.num_match",  getspec_long,  (char *) &parts_match,
    TRACE_PARAM ("retrieve.local.vec_vec.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int textloc_fd;
static int f_pv = UNDEF;

static int num_inst = 0;

#define Malloc(n,type) (type *) malloc( (unsigned) (n)*sizeof(type) )
static int get_part_vec();
static int save_part_vec();
static int free_part_vec();

static PART_VEC pvec1;
static PART_VEC pvec2;

int
init_local_partvec (spec, unused)
SPEC *spec;
char *unused;
{
    CONTEXT old_context;

    if (num_inst++ > 0)
        return (num_inst);

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_local_partvec");

    /*
     * We either get the partvecs from a file, or we compute them on 
     * the fly, which requires the textloc file and a preparser.
     */
    if ( VALID_FILE(partvec_file) ) {
	if (UNDEF == (f_pv = open_partvec( partvec_file, partvec_mode )))
	    return UNDEF;
    }
    else {
	if (UNDEF==(textloc_fd = open_textloc( textloc_file, textloc_mode )) ||
	    UNDEF==(preparse.inst=preparse.ptab->init_proc( spec, NULL )) ||
	    UNDEF==(make_pvec.inst=make_pvec.ptab->init_proc( spec, NULL )))
	    return UNDEF;
    }

    old_context = get_context();
    set_context (CTXT_DOC);
    if (UNDEF==(parts_compare.inst = parts_compare.ptab->init_proc(spec,NULL)))
        return (UNDEF);

    set_context (old_context);

    /* Initialize cached pvec's */
    pvec1.id_num = pvec2.id_num = UNDEF;

    PRINT_TRACE (8, print_float, &parts_thresh);
    PRINT_TRACE (8, print_long, &parts_match);

    PRINT_TRACE (2, print_string, "Trace: leaving init_local_partvec");
    return (num_inst);
}

int
local_partvec (vec_pair, sim, inst)
VEC_PAIR *vec_pair;
float *sim;
int inst;
{
    PART_VEC_PAIR part_vec_pair;
    ALT_RESULTS sim_results;
    long num_match;
    long i;
    float max_sim;

    PRINT_TRACE (2, print_string, "Trace: entering local_partvec");
    PRINT_TRACE (6, print_long, &vec_pair->vec1->id_num);
    PRINT_TRACE (6, print_long, &vec_pair->vec2->id_num);

    if (vec_pair->vec1->id_num != pvec1.id_num) {
        if (pvec1.id_num != UNDEF)
            (void) free_part_vec( &pvec1 );
        if (UNDEF == get_part_vec(vec_pair->vec1->id_num, &pvec1) ||
            UNDEF == save_part_vec(&pvec1))
            return (UNDEF);
    }
    part_vec_pair.pvec1 = &pvec1;

    if (vec_pair->vec2->id_num != pvec2.id_num) {
        if (pvec2.id_num != UNDEF)
            (void) free_part_vec (&pvec2);
        if (UNDEF == get_part_vec (vec_pair->vec2->id_num, &pvec2) ||
            UNDEF == save_part_vec (&pvec2))
            return (UNDEF);
    }
    part_vec_pair.pvec2 = &pvec2;

    if (UNDEF == parts_compare.ptab->proc (&part_vec_pair,
					   &sim_results,
					   parts_compare.inst))
        return (UNDEF);

    num_match = 0;
    max_sim = 0.0;
    for (i = 0; i < sim_results.num_results; i++) {
        if (sim_results.results[i].sim >= parts_thresh)
            num_match++;
        if (sim_results.results[i].sim > max_sim)
            max_sim = sim_results.results[i].sim;
    }

    PRINT_TRACE (8, print_float, &max_sim);
    PRINT_TRACE (8, print_long, &num_match);

    if (num_match < parts_match)
        *sim = 0.0;
    else
        *sim = (float) max_sim;

    PRINT_TRACE (4, print_float, sim);
    PRINT_TRACE (2, print_string, "Trace: leaving local_partvec");
    return (1);
}


int
close_local_partvec (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_local_partvec");

    if (--num_inst) {
        PRINT_TRACE (2, print_string, "Trace: leaving close_local_partvec");
        return (0);
    }

    if (f_pv == UNDEF) {
	if (UNDEF == make_pvec.ptab->close_proc( make_pvec.inst ) ||
	    UNDEF == preparse.ptab->close_proc( preparse.inst ) ||
	    UNDEF == close_textloc( textloc_fd ))
	    return UNDEF;
    }
    else
	if (UNDEF == close_partvec( f_pv ))
	    return UNDEF;

    if (UNDEF == parts_compare.ptab->close_proc (parts_compare.inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_local_partvec");
    return (1);
}



/******************************************************************
 *
 * Get the part_vec corresponding to the passed docid.
 *
 ******************************************************************/
static int
get_part_vec (did, part_vec)
long did;
PART_VEC *part_vec;
{
    if (f_pv != UNDEF) {
	part_vec->id_num = did;
	if (1 != seek_partvec( f_pv, part_vec ) ||
	    1 != read_partvec( f_pv, part_vec ))
	    return UNDEF;
    }
    else {
	TEXTLOC textloc;
	SM_INDEX_TEXTDOC textdoc;

	textloc.id_num = did;
	textdoc.id_num = did;
	if (1 != seek_textloc (textloc_fd, &textloc) ||
	    1 != read_textloc (textloc_fd, &textloc) ||
	    UNDEF == preparse.ptab->proc (&textloc, &textdoc,
					  preparse.inst) ||
	    UNDEF == make_pvec.ptab->proc( &textdoc, part_vec,
					   make_pvec.inst ))
	    return UNDEF;
    }
    return 1;
}


static int
save_part_vec( pvec )
PART_VEC *pvec;
{
    long *saved_ctype_len;
    PART_CON_WT *saved_pconwt;

    if (NULL == (saved_ctype_len = Malloc( pvec->num_ctype, long )) ||
	NULL == (saved_pconwt = Malloc( pvec->num_part_conwt, PART_CON_WT )))
	return UNDEF;

    bcopy( (char *) pvec->ctype_len,
	   (char *) saved_ctype_len,
	   (int) pvec->num_ctype * sizeof(long) );
    bcopy( (char *) pvec->part_con_wtp,
	   (char *) saved_pconwt,
	   (int) pvec->num_part_conwt * sizeof(PART_CON_WT) );

    pvec->ctype_len = saved_ctype_len;
    pvec->part_con_wtp = saved_pconwt;
    return 1;
}

static int
free_part_vec( pvec )
PART_VEC *pvec;
{
    (void) free( (char *) pvec->ctype_len );
    (void) free( (char *) pvec->part_con_wtp );

    pvec->ctype_len = NULL;
    pvec->part_con_wtp = NULL;
    return 1;
}
