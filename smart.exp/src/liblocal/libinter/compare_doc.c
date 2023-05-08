#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libinter/compare_doc.c.,v 11.0 1992/07/21 18:22:14 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/* Note: in Procedure description, every content line must begin with
   the appropriate " *<digit> ".  Try not to leave blank lines within
   a numbered section.
*/
/********************   PROCEDURE DESCRIPTION   ************************
 *0 Compare two documents in many, many ways
 *2 compare_docs(is,unused)
 *3    INTER_STATE *is;
 *3    char *unused;
 *4 init_compare_docs(spec,unused)
 *5    "inter.compare.partvecs_vecs"
 *5    "inter.compare.vec_vec"
 *5
 *5    "inter.compare.sect.preparse"	This set used only if one (or more)
 *5    "inter.compare.para.preparse"	of the partvec_file's below is
 *5    "inter.compare.sent.preparse"	invalid.
 *5    "inter.compare.get_partvec"
 *5    "doc.textloc_file"
 *5    "doc.textloc_file.rmode"
 *5
 *5    "inter.compare.num_compares"	'*' below is 0..(num_compares - 1)
 *5    "inter.compare.*.left.part_type"
 *5    "inter.compare.*.right.part_type"
 *5    "inter.compare.*.list_max"
 *5    "inter.compare.*.list_thresh"
 *5    "inter.compare.*.left.partvec_file"
 *5    "inter.compare.*.left.partvec_file.rmode"
 *5    "inter.compare.*.right.partvec_file"
 *5    "inter.compare.*.right.partvec_file.rmode"

 *4 close_compare_docs(inst)

 *7 Get the two documents listed and make the following comparisons
 *7 between them.  Return 0 for an error; return 1 otherwise.

***********************************************************************/

#include <strings.h>
extern char *strdup();

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "io.h"
#include "inst.h"
#include "proc.h"
#include "inter.h"
#include "context.h"
#include "section.h"
#include "part_vector.h"

#define NUM_PART_TYPES 3
#define PART_SECT 0
#define PART_PARA 1
#define PART_SENT 2
#define PART_UNKNOWN -1

static PROC_INST comp_pvecs;
static PROC_INST comp_vec_pair;

static char *textloc_file;
static long textloc_mode;
static PROC_INST preparser[NUM_PART_TYPES];
static PROC_INST make_pvec;

static long num_compares;

static SPEC_PARAM spec_args[] = {
    "inter.compare.partvecs_vecs",getspec_func, (char *) &comp_pvecs.ptab,
    "inter.vec_vec",		getspec_func,	(char *) &comp_vec_pair.ptab,

    "inter.compare.sect.preparse", getspec_func,
                            (char *) &preparser[PART_SECT].ptab,
    "inter.compare.para.preparse", getspec_func,
                            (char *) &preparser[PART_PARA].ptab,
    "inter.compare.sent.preparse", getspec_func,
                            (char *) &preparser[PART_SENT].ptab,
    "inter.compare.get_partvec", getspec_func, (char *) &make_pvec.ptab,
    "doc.textloc_file", getspec_dbfile, (char *) &textloc_file,
    "doc.textloc_file.rmode", getspec_filemode, (char *) &textloc_mode,

    "inter.compare.num_compares",getspec_long,	(char *) &num_compares,

    TRACE_PARAM ("inter.compare.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);


/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

} STATIC_INFO;

static int textloc_fd;
static SM_INDEX_TEXTDOC textdoc[2][NUM_PART_TYPES];

static STATIC_INFO *info;
static int max_inst = 0;

typedef struct {
    char part_type[2];
    int pv_fd[2];		/* partvec fd; UNDEF if unavailable */
    long make_pvec_inst[2];	/* for when partvec file unavailable */
    long list_max;
    float list_thresh; } COMPARE;
static COMPARE *cmp;

static int get_vec();
static void find_matches();
static void find_pvec_matches();
static int save_index_textdoc();
static int free_index_textdoc();
static int compare_res_sim();
static int get_pvec();
static int save_pvec();
static int free_pvec();

#define PART_NAME(n) (n)==0 ? "left" : "right"
#define LEFT 0
#define RIGHT 1

static int need_textdoc;
static int used_part[NUM_PART_TYPES];

#define Malloc(n,type) (type *) malloc( (unsigned) (n)*sizeof(type) )

#define PTYPE_TO_CODE(p) (strcasecmp(p,"sect")==0 ? PART_SECT : \
			  strcasecmp(p,"para")==0 ? PART_PARA : \
			  strcasecmp(p,"sent")==0 ? PART_SENT : PART_UNKNOWN)

#define CODE_TO_PTYPE(c) ((c)==PART_SECT ? "SECTION" : \
			  (c)==PART_PARA ? "PARAGRAPH" : \
			  (c)==PART_SENT ? "SENTENCE" : "?" )

#define CODE_TO_ptype(c) ((c)==PART_SECT ? "sect" : \
			  (c)==PART_PARA ? "para" : \
			  (c)==PART_SENT ? "sent" : "?" )

/******************************************************************
 *
 * Init.
 *
 ******************************************************************/
int
init_compare_docs (spec, unused)
SPEC *spec;
char *unused;
{
    STATIC_INFO *ip;
    int new_inst;
    long i, c;
    char param[PATH_LEN];
    char *ptype;
    long old_context;
    SPEC_PARAM sparam;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (1, print_string, "Trace: entering init_compare_docs");
    
    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);
    
    ip = &info[new_inst];

    if (NULL == (cmp = Malloc( num_compares, COMPARE )))
	return UNDEF;

    need_textdoc = 0;
    for (i=0; i<NUM_PART_TYPES; i++)
	used_part[i] = 0;

    sparam.param = param;

    /*
     * For each of the comparisons, we need the following information:
     *
     * inter.compare.N.left.part_type
     * inter.compare.N.right.part_type
     *		Indicates which is the left and right parts that
     *		are being compared (e.g., paras v. sents)
     * inter.compare.N.list_max
     * inter.compare.N.list_thresh
     *		Chooses maximum number of pairwise similarities
     *		to list; in no case will anything with sim below
     *		the threshold be listed.
     * inter.compare.N.left.partvec_file
     * inter.compare.N.right.partvec_file
     * inter.compare.N.left.partvec_file.rmode
     * inter.compare.N.right.partvec_file.rmode
     *		Location to get pre-calculated part-vectors for
     *		this side.  If a file is invalid then the part_type
     *		parameter is used along with the following to
     *		build the part-vector.
     * doc.textloc_file
     * doc.textloc_file.rmode
     * inter.compare.sect.preparse
     * inter.compare.para.preparse
     * inter.compare.sent.preparse
     * inter.compare.get_partvec
     *		These parameters were evaluated above.  They determine
     *		how a part vector can be built.  First the textloc_file
     *		is used to retrieve the original text.  Then it is
     *		preparsed appropriately (into sects, paras, etc).  Then
     *		it is turned into a part vector.  The get_partvec routine
     *		is called with a prefix of "inter.compare.N." so that
     *		"*.token" and "*.parse" and "*.weight" can be used
     *		to build and weight the vector appropriately.
     */
    for (c=0; c<num_compares; c++) {
	for (i=0; i<2; i++) {
	    sparam.convert = getspec_string;
	    sparam.result = (char *) &ptype;
	    (void) sprintf( param, "inter.compare.%d.%s.part_type",
			    c, PART_NAME(i) );
	    if (UNDEF == (lookup_spec( spec, sparam, 1 )))
		return UNDEF;
	    cmp[c].part_type[i] = PTYPE_TO_CODE(ptype);
	    used_part[ cmp[c].part_type[i] ]++;
	}
	if (cmp[c].part_type[LEFT] == PART_UNKNOWN ||
	    cmp[c].part_type[RIGHT] == PART_UNKNOWN ) {
	    set_error( SM_ILLPA_ERR, "part_type unrecognized", "compare" );
	    return UNDEF;
	}

	/*
	 * Now the list sizes.
	 */
	sparam.convert = getspec_long;
	sparam.result = (char *) &cmp[c].list_max;
	(void) sprintf( param, "inter.compare.%d.list_max", c );
	if (UNDEF == (lookup_spec( spec, sparam, 1 )))
	    return UNDEF;

	sparam.convert = getspec_float;
	sparam.result = (char *) &cmp[c].list_thresh;
	(void) sprintf( param, "inter.compare.%d.list_thresh", c );
	if (UNDEF == (lookup_spec( spec, sparam, 1 )))
	    return UNDEF;

	/*
	 * Now the partvec files.
	 */
	for (i=0; i<2; i++) {
	    char *partvec_file;
	    long partvec_mode;

	    sparam.convert = getspec_dbfile;
	    sparam.result = (char *) &partvec_file;
	    (void) sprintf( param, "inter.compare.%d.%s.partvec_file",
			    c, PART_NAME(i) );
	    if (UNDEF == (lookup_spec( spec, sparam, 1 )))
		return UNDEF;

	    sparam.convert = getspec_long;
	    sparam.result = (char *) &partvec_mode;
	    (void) sprintf( param, "inter.compare.%d.%s.partvec_file.rmode",
			    c, PART_NAME(i) );
	    if (UNDEF == (lookup_spec( spec, sparam, 1 )))
		return UNDEF;

	    if (VALID_FILE( partvec_file )) {
		if (UNDEF == (cmp[c].pv_fd[i] = open_partvec( partvec_file,
							      partvec_mode )))
		    return UNDEF;
	    }
	    else
		cmp[c].pv_fd[i] = UNDEF;
	}

	/*
	 * Now, if no partvec file was given, initialize the
	 * partvec creation routine.
	 */
	(void) sprintf( param, "inter.compare.%d.", c );
	old_context = get_context();

	for (i=0; i<2; i++) {
	    if ( cmp[c].pv_fd[i] != UNDEF )
		continue;

	    switch ( cmp[c].part_type[i] ) {
	      case PART_SECT:
		add_context( CTXT_SECT ); break;
	      case PART_PARA:
		add_context( CTXT_PARA ); break;
	      case PART_SENT:
		add_context( CTXT_SENT ); break;
	    }

	    need_textdoc = 1;
	    if (UNDEF == (cmp[c].make_pvec_inst[i] =
			     make_pvec.ptab->init_proc(spec,param)))
		return UNDEF;
	    set_context( old_context );
	}
    } /* loop through compares */

    if (UNDEF == (comp_pvecs.inst =
		           comp_pvecs.ptab->init_proc(spec,NULL)) ||
	UNDEF == (comp_vec_pair.inst =
		           comp_vec_pair.ptab->init_proc(spec,NULL)))
	return UNDEF;

    if (need_textdoc) {
	for (i=0; i<NUM_PART_TYPES; i++)
	    if ( used_part[i] ) /* don't init if we won't use it */
		if (UNDEF == (preparser[i].inst =
			      preparser[i].ptab->init_proc(spec,NULL)))
		    return UNDEF;
	if (UNDEF==(textloc_fd = open_textloc( textloc_file, textloc_mode )) ||
	    UNDEF==(make_pvec.inst = make_pvec.ptab->init_proc(spec, NULL )))
	    return UNDEF;
    }

    ip->valid_info = 1;
             
    PRINT_TRACE (1, print_string, "Trace: leaving init_compare_docs");
    return (new_inst);
}

/******************************************************************
 *
 * Do the work.
 *
 ******************************************************************/
int
compare_docs(is,unused)
INTER_STATE *is;
char *unused;
{
    char temp[100];
    VEC vec[2];
    VEC_PAIR vec_pair;
    int status;
    long c, i, j;
    TEXTLOC textloc[2];
    float sim, pct_sim_above_avg;
    long num_in_common, num_above_avg;

    int over_thresh, list_cnt;

    PRINT_TRACE (1, print_string, "Trace: entering compare_docs");

    if (is->num_command_line < 3) {
	if (UNDEF == add_buf_string( "Need two docids to compare\n",
				     &is->err_buf ))
	    return UNDEF;
	return 0;
    }

    /*
     * Get document vectors and the textloc (for the titles).  Then,
     * if we need to do any preparsing (since we're missing a partvec
     * file), get the textdoc also.
     */
    if (1 != ( status = get_vec( is, is->command_line[1], &vec[LEFT] )))
	return status;
    if (UNDEF == save_vec( &vec[LEFT] ))
	return UNDEF;
    if (1 != ( status = get_vec( is, is->command_line[2], &vec[RIGHT] )))
	return status;

    for (i=0; i<2; i++) {
	if (UNDEF == inter_get_textloc( vec[i].id_num, &textloc[i] ))
	    return UNDEF;

	if (need_textdoc)
	    for (j=0; j<NUM_PART_TYPES; j++) {
		if ( used_part[j] )
		    if (UNDEF == preparser[j].ptab->proc( &textloc[i],
							 &textdoc[i][j],
							 preparser[j].inst ) ||
			UNDEF == save_index_textdoc( &textdoc[i][j] ))
			return UNDEF;
	    }
    }

    /*
     * Get information about overall document similarity.
     */
    vec_pair.vec1 = &vec[LEFT];
    vec_pair.vec2 = &vec[RIGHT];
    if (UNDEF == comp_vec_pair.ptab->proc( &vec_pair, &sim,
					  comp_vec_pair.inst ))
	return UNDEF;
    find_matches( vec[LEFT], vec[RIGHT], sim,
		  &num_in_common, &num_above_avg,
		 &pct_sim_above_avg );

    /*
     * Display overall document information.
     */
    if (UNDEF == add_buf_string( "Comparison of documents\n",&is->output_buf ))
	return UNDEF;

    for (i=0; i<2; i++) {
	(void) sprintf( temp, "%6d %s\n", vec[i].id_num,
		       textloc[i].title );
	if (UNDEF == add_buf_string( temp, &is->output_buf ))
	    return UNDEF;
    }
    (void) sprintf( temp, "\nSimilarity of %6.4f\n", sim );
    if (UNDEF == add_buf_string( temp, &is->output_buf ))
	return UNDEF;
		   
    (void) sprintf( temp, "%d concept%s match%s",
		   num_in_common,
		   num_in_common==1 ? "" : "s",
		   num_in_common==1 ? "es" : "" );
    if (UNDEF == add_buf_string( temp, &is->output_buf ))
	return UNDEF;

    if (num_in_common <= 1)
	(void) sprintf( temp, "\n" );
    else if ( num_above_avg == num_in_common )
	(void) sprintf( temp, "; evenly distributed\n" );
    else
	(void) sprintf( temp, "; %d make%s up %1.1f%% of sim\n",
		       num_above_avg,
		       num_above_avg==1 ? "s" : "",
		       pct_sim_above_avg );
    if (UNDEF == add_buf_string( temp, &is->output_buf ))
	return UNDEF;

    /* No point in doing anything else in this case. */
    if (num_in_common == 0) {
	if (UNDEF == add_buf_string( "\n(part comparison skipped)",
				    &is->output_buf))
	    return UNDEF;

	PRINT_TRACE (1, print_string, "Trace: leaving compare_docs");
	return 1;
    }

    /*
     * Now go through the various part-part comparisons and
     * do the work.
     */
    for (c=0; c<num_compares; c++) {
	PART_VEC pvec[2];
	PART_VEC_PAIR pvec_pair;
	ALT_RESULTS results;

	if (UNDEF ==get_pvec( vec[LEFT].id_num, &cmp[c],LEFT, &pvec[LEFT] ) ||
	    UNDEF ==save_pvec( &pvec[LEFT] ) ||
	    UNDEF ==get_pvec( vec[RIGHT].id_num, &cmp[c], RIGHT, &pvec[RIGHT]))
	    return UNDEF;

	pvec_pair.pvec1 = &pvec[LEFT];
	pvec_pair.pvec2 = &pvec[RIGHT];
	if (UNDEF == comp_pvecs.ptab->proc( &pvec_pair,
					   &results,
					   comp_pvecs.inst ))
	    return UNDEF;

	/* Sort sim_results by decreasing sim */
	qsort ((char *) results.results,
	       (int) results.num_results,
	       sizeof (RESULT_TUPLE),
	       compare_res_sim);

	/*
	 * Now display the information.
	 */
	(void) sprintf( temp, "\n%s-%s COMPARISON (%d %s%s in %d;"
		                      " %d %s%s in %d)\n",
		       CODE_TO_PTYPE( cmp[c].part_type[LEFT] ),
		       CODE_TO_PTYPE( cmp[c].part_type[RIGHT] ),
		       pvec[LEFT].partnums_used,
		       CODE_TO_ptype( cmp[c].part_type[LEFT] ),
		       pvec[LEFT].partnums_used==1 ? "" : "s",
		       vec[LEFT].id_num,
		       pvec[RIGHT].partnums_used,
		       CODE_TO_ptype( cmp[c].part_type[RIGHT] ),
		       pvec[RIGHT].partnums_used==1 ? "" : "s",
		       vec[RIGHT].id_num );
	if (UNDEF == add_buf_string( temp, &is->output_buf ))
	    return UNDEF;

	(void) sprintf( temp,
		       "  %d out of %d pairs have non-zero similarity.\n",
		       results.num_results,
		       pvec[LEFT].partnums_used * pvec[RIGHT].partnums_used );
	if (UNDEF == add_buf_string( temp, &is->output_buf ))
	    return UNDEF;

	over_thresh = list_cnt = 0;
	for (i=0; i<results.num_results &&
	          results.results[i].sim >= cmp[c].list_thresh; i++) {
	    over_thresh++;
	    list_cnt++;
	}

	if (over_thresh==0)
	    (void) sprintf( temp, "  No pair exceeds %.2f\n",
			   cmp[c].list_thresh );
	else if (over_thresh <= cmp[c].list_max)
	    (void) sprintf( temp,
			   "  All those with similarity exceeding %.2f:\n\n",
			   cmp[c].list_thresh );
	else {
	    (void) sprintf( temp,
			   "  Top %d of %d with sim exceeding %.2f:\n\n",
			   cmp[c].list_max,
			   over_thresh,
			   cmp[c].list_thresh );
	    list_cnt = cmp[c].list_max;
	}
	if (UNDEF == add_buf_string( temp, &is->output_buf ))
	    return UNDEF;
	
	for (i=0; i<list_cnt; i++) {
	    long d1, d2;

	    d1 = results.results[i].qid;
	    d2 = results.results[i].did;
	    sim = results.results[i].sim;

	    find_pvec_matches( &pvec[LEFT], &pvec[RIGHT],
			       d1, d2, sim,
			       &num_in_common, &num_above_avg,
			       &pct_sim_above_avg );

	    (void) sprintf( temp,
			   "   %2d. %-6d %-6d %9.4f %3d concept%s match%s",
			   i+1, d1, d2, sim,
			   num_in_common,
			   num_in_common==1 ? "" : "s",
			   num_in_common==1 ? "es" : "" );
	    if (UNDEF == add_buf_string( temp, &is->output_buf ))
		return UNDEF;

	    if (num_in_common <= 1)
		(void) sprintf( temp, "\n" );
	    else if ( num_above_avg == num_in_common )
		(void) sprintf( temp, "; evenly distributed\n" );
	    else
		(void) sprintf( temp, "; %d make%s up %1.1f%% of sim\n",
			        num_above_avg,
			       num_above_avg==1 ? "s" : "",
			       pct_sim_above_avg );
	    if (UNDEF == add_buf_string( temp, &is->output_buf ))
		return UNDEF;
	} /* end of similarities. */
	    
	if (UNDEF == free_pvec( &pvec[LEFT] ))
	    return UNDEF;

    } /* end of comparisons */

    if (need_textdoc)
	for (i=0; i<2; i++)
	    for (j=0; j<NUM_PART_TYPES; j++)
		if ( used_part[j] )
		    if (UNDEF == free_index_textdoc( &textdoc[i][j] ))
			return UNDEF;
    free_vec( &vec[LEFT] );

    PRINT_TRACE (1, print_string, "Trace: leaving compare_docs");
    return (1);
}

/******************************************************************
 *
 * Close up shop.
 *
 ******************************************************************/
int
close_compare_docs(inst)
int inst;
{
    STATIC_INFO *ip;
    long c, i;

    PRINT_TRACE (1, print_string, "Trace: entering close_compare_docs");
    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_compare_docs");
        return (UNDEF);
    }

    for (c=0; c<num_compares; c++)
	for (i=0; i<2; i++) {
	    if ( cmp[c].pv_fd[i] == UNDEF ) {
		if (UNDEF==(make_pvec.ptab->close_proc(
						cmp[c].make_pvec_inst[i])))
		    return UNDEF;
	    }
	    else
		if (UNDEF == close_partvec( cmp[c].pv_fd[i] ))
		    return UNDEF;
	}
    (void) free( cmp );

    if (UNDEF == comp_pvecs.ptab->close_proc( comp_pvecs.inst ) ||
	UNDEF == comp_vec_pair.ptab->close_proc( comp_vec_pair.inst ))
	return UNDEF;

    /*
     * Shut down the routines used to build part-vectors if there is
     * no pre-built partvec file.
     */
    if (need_textdoc) {
	for (i=0; i<NUM_PART_TYPES; i++)
	    if ( used_part[i] )
		if (UNDEF == preparser[i].ptab->close_proc(preparser[i].inst))
		    return UNDEF;
	if (UNDEF == make_pvec.ptab->close_proc( make_pvec.inst ) ||
            UNDEF == close_textloc( textloc_fd ))
            return UNDEF;
    }

    ip  = &info[inst];
    ip->valid_info--;

    PRINT_TRACE (1, print_string, "Trace: leaving close_compare_docs");
    return (0);
}


/******************************************************************
 *
 * Get the vector associated with a document and ensure that it's
 * only a single vector.
 *
 ******************************************************************/
static int
get_vec (is, command_line, vec)
INTER_STATE *is;
char *command_line;
VEC *vec;
{
    static VEC_LIST vec_list;  /* static to avoid memory leak */
    static VEC temp_vec;       /* successive calls would cause */

    vec_list.vec = &temp_vec;

    if (UNDEF == inter_get_sect_veclist (command_line, &vec_list) ||
        vec_list.num_vec != 1) {
        if (UNDEF == add_buf_string ("Must be a single document; "
				               "command ignored\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    *vec = vec_list.vec[0];
    return (1);
}


/******************************************************************
 *
 * Scan a pair of vectors to find the concepts in common.
 * Also figure out how many of them are above the average similarity
 * those terms would have if the total similarity were evenly
 * divided among all the terms.
 *
 ******************************************************************/
static void
find_matches( vec1, vec2, sim, num_in_common, num_above_avg,pct_sim_above_avg )
VEC *vec1, *vec2;
float sim;
int *num_in_common;
int *num_above_avg;
float *pct_sim_above_avg;
{
    long ctype;                /* Current ctype checking for match */
    CON_WT *conwt1, *conwt2;   /* Current concepts checking */
    CON_WT *end_ctype1, *end_ctype2; /* Last concept for this ctype */

    int common_count = 0, above_avg_count = 0;
    float avg_sim, this_wt, above_avg_totsim = 0.0;
    /*
     * Count the number of concepts in common.
     */
    conwt1 = vec1->con_wtp;
    conwt2 = vec2->con_wtp;
    for (ctype = 0;
         ctype < vec1->num_ctype && ctype < vec2->num_ctype;
         ctype++) {
        end_ctype1 = &conwt1[vec1->ctype_len[ctype]];
        end_ctype2 = &conwt2[vec2->ctype_len[ctype]];
        while (conwt1 < end_ctype1 && conwt2 < end_ctype2) {
            if (conwt1->con < conwt2->con)
                conwt1++;
            else if (conwt1->con > conwt2->con)
                conwt2++;
            else {
		common_count++;
		conwt1++;
		conwt2++;
	    }
        }
        conwt1 = end_ctype1;
        conwt2 = end_ctype2;
    }

    *num_in_common = common_count;
    if (common_count==0) {
	*num_above_avg = 0;
	*pct_sim_above_avg = 0.0;
	return;
    }

    /*
     * Now scan again to see how well they're distributed.
     */
    avg_sim = sim / (float)common_count;
    conwt1 = vec1->con_wtp;
    conwt2 = vec2->con_wtp;
    for (ctype = 0;
         ctype < vec1->num_ctype && ctype < vec2->num_ctype;
         ctype++) {
        end_ctype1 = &conwt1[vec1->ctype_len[ctype]];
        end_ctype2 = &conwt2[vec2->ctype_len[ctype]];
        while (conwt1 < end_ctype1 && conwt2 < end_ctype2) {
            if (conwt1->con < conwt2->con)
                conwt1++;
            else if (conwt1->con > conwt2->con)
                conwt2++;
            else {
		this_wt = conwt1->wt * conwt2->wt;
		if ( this_wt > avg_sim ) {
		    above_avg_count++;
		    above_avg_totsim += this_wt;
		}
		conwt1++;
		conwt2++;
	    }
        }
        conwt1 = end_ctype1;
        conwt2 = end_ctype2;
    }

    *num_above_avg = above_avg_count;
    if (above_avg_count == 0)  /* this seems pretty unlikely, but... */
	*pct_sim_above_avg = 0.0;
    else
	*pct_sim_above_avg = 100.0 * above_avg_totsim / sim;
}


/******************************************************************
 *
 * Same as find_matches, but intended for part vectors.  The part
 * numbers of the two pvecs are also passed in.
 *
 ******************************************************************/
static void
find_pvec_matches( pvec1, pvec2, pnum1, pnum2,
		   sim, num_in_common,
		   num_above_avg,pct_sim_above_avg )
PART_VEC *pvec1, *pvec2;
long pnum1, pnum2;
float sim;
int *num_in_common;
int *num_above_avg;
float *pct_sim_above_avg;
{
    long ctype;                /* Current ctype checking for match */
    PART_CON_WT *conwt1, *conwt2;   /* Current concepts checking */
    PART_CON_WT *end_ctype1, *end_ctype2; /* Last concept for this ctype */

    int common_count = 0, above_avg_count = 0;
    float avg_sim, this_wt, above_avg_totsim = 0.0;
    /*
     * Count the number of concepts in common.
     */
    conwt1 = pvec1->part_con_wtp;
    conwt2 = pvec2->part_con_wtp;
    for (ctype = 0;
         ctype < pvec1->num_ctype && ctype < pvec2->num_ctype;
         ctype++) {
        end_ctype1 = &conwt1[pvec1->ctype_len[ctype]];
        end_ctype2 = &conwt2[pvec2->ctype_len[ctype]];
        while (conwt1 < end_ctype1 && conwt2 < end_ctype2) {
            if (conwt1->con < conwt2->con || conwt1->partnum != pnum1 )
                conwt1++;
            else if (conwt1->con > conwt2->con || conwt2->partnum != pnum2 )
                conwt2++;
            else {
		common_count++;
		conwt1++;
		conwt2++;
	    }
        }
        conwt1 = end_ctype1;
        conwt2 = end_ctype2;
    }

    *num_in_common = common_count;
    if (common_count==0) {
	*num_above_avg = 0;
	*pct_sim_above_avg = 0.0;
	return;
    }

    /*
     * Now scan again to see how well they're distributed.
     */
    avg_sim = sim / (float)common_count;
    conwt1 = pvec1->part_con_wtp;
    conwt2 = pvec2->part_con_wtp;
    for (ctype = 0;
         ctype < pvec1->num_ctype && ctype < pvec2->num_ctype;
         ctype++) {
        end_ctype1 = &conwt1[pvec1->ctype_len[ctype]];
        end_ctype2 = &conwt2[pvec2->ctype_len[ctype]];
        while (conwt1 < end_ctype1 && conwt2 < end_ctype2) {
            if (conwt1->con < conwt2->con || conwt1->partnum != pnum1 )
                conwt1++;
            else if (conwt1->con > conwt2->con || conwt2->partnum != pnum2 )
                conwt2++;
            else {
		this_wt = conwt1->wt * conwt2->wt;
		if ( this_wt > avg_sim ) {
		    above_avg_count++;
		    above_avg_totsim += this_wt;
		}
		conwt1++;
		conwt2++;
	    }
        }
        conwt1 = end_ctype1;
        conwt2 = end_ctype2;
    }

    *num_above_avg = above_avg_count;
    if (above_avg_count == 0)  /* this seems pretty unlikely, but... */
	*pct_sim_above_avg = 0.0;
    else
	*pct_sim_above_avg = 100.0 * above_avg_totsim / sim;
}


#define STRDUP(s) (s)==NULL ? NULL : strdup(s)

/******************************************************************
 *
 * Routines to save and free an "sm_index_textdoc" document so that
 * more than one can be present.  Note that the free routine
 * assumes that it is freeing the result of the save routine; do not
 * use the free routine to free a textdoc created elsewhere.
 *
 ******************************************************************/
static int
save_index_textdoc( t )
SM_INDEX_TEXTDOC *t;
{
    char *c_doctext;
    char *tl_file_name, *tl_title;
    char *md_file_name, *md_title;
    SM_DISP_SEC *md_sects, *md_sects_p;
    long i;
    int num_sects = t->mem_doc.num_sections;
    int doc_text_len = t->mem_doc.sections[num_sects-1].end_section;
    
    if (NULL == (c_doctext = Malloc( doc_text_len+1, char )) ||
	NULL == (md_sects = Malloc( t->mem_doc.num_sections,SM_DISP_SEC)))
	return UNDEF;


    bcopy( t->doc_text, c_doctext, doc_text_len );
    t->doc_text = c_doctext;
    t->doc_text[doc_text_len] = '\0';  /* just in case */

    tl_file_name = STRDUP( t->textloc_doc.file_name );
    t->textloc_doc.file_name = tl_file_name;
    tl_title = STRDUP( t->textloc_doc.title );
    t->textloc_doc.title = tl_title;

    md_file_name = STRDUP( t->mem_doc.file_name );
    t->mem_doc.file_name = md_file_name;
    md_title = STRDUP( t->mem_doc.title );
    t->mem_doc.title = md_title;

    md_sects_p = md_sects;
    for (i=0; i<t->mem_doc.num_sections; i++)
	*md_sects_p++ = t->mem_doc.sections[i];  /* not guaranteed adjacent */
    t->mem_doc.sections = md_sects;

    return 1;
}


static int	
free_index_textdoc( t )
SM_INDEX_TEXTDOC *t;
{

    if (t->doc_text != NULL)
	(void) free( t->doc_text );

    if (t->textloc_doc.file_name != NULL)
	(void) free( t->textloc_doc.file_name );
    if (t->textloc_doc.title != NULL)
	(void) free( t->textloc_doc.title );

    if (t->mem_doc.sections != NULL)
	(void) free( t->mem_doc.sections );  /* assume alloc'd as a chunk */
    if (t->mem_doc.file_name != NULL)
	(void) free( t->mem_doc.file_name );
    if (t->mem_doc.title != NULL)
	(void) free( t->mem_doc.title );

    return 1;
}


/******************************************************************
 *
 * Comparison routine for quicksort.
 *
 ******************************************************************/
static int
compare_res_sim (ptr1, ptr2)
RESULT_TUPLE *ptr1, *ptr2;
{
    if (ptr1->sim > ptr2->sim)
        return (-1);
    if (ptr1->sim < ptr2->sim)
        return (1);
    return (0);
}



/******************************************************************
 *
 * Retrieve a partvec, either from a file, or by getting the
 * actual text.
 *
 ******************************************************************/
static int
get_pvec( docid, c, lr, pvec )
long docid;
COMPARE *c;
int lr;
PART_VEC *pvec;
{

    if ( c->pv_fd[lr] == UNDEF ) {
	if (UNDEF == make_pvec.ptab->proc( &textdoc[lr][c->part_type[lr]],
					    pvec,
					    c->make_pvec_inst[lr] ))
		return UNDEF;
    }
    else {
	pvec->id_num = docid;
	if (1 != seek_partvec( c->pv_fd[lr], pvec ) ||
	    1 != read_partvec( c->pv_fd[lr], pvec ))
	    return UNDEF;
    }

    return 1;
}


static int
save_pvec( pvec )
PART_VEC *pvec;
{
    long *saved_ctype_len;
    PART_CON_WT *saved_pconwt;

    if (NULL == (saved_ctype_len = Malloc( pvec->num_ctype, long )) ||
        NULL == (saved_pconwt = Malloc( pvec->num_part_conwt, PART_CON_WT )))
        return UNDEF;

    bcopy( (char *) pvec->ctype_len,
           (char *) saved_ctype_len,
           pvec->num_ctype * sizeof(long) );
    bcopy( (char *) pvec->part_con_wtp,
           (char *) saved_pconwt,
           pvec->num_part_conwt * sizeof(PART_CON_WT) );

    pvec->ctype_len = saved_ctype_len;
    pvec->part_con_wtp = saved_pconwt;
    return 1;
}

static int
free_pvec( pvec )
PART_VEC *pvec;
{
    (void) free( (char *) pvec->ctype_len );
    (void) free( (char *) pvec->part_con_wtp );

    pvec->ctype_len = NULL;
    pvec->part_con_wtp = NULL;
    return 1;
}
