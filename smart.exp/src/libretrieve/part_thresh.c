#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/part_thresh.c,v 11.0 1992/07/21 18:23:49 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Calculate a sim between vectors based on sim between vector parts
 *1 retrieve.vec_vec.part_thresh
 *2 part_thresh (vec_pair, sim, inst)
 *3   VEC_PAIR *vec_pair;
 *3   float *sim;
 *3   int inst;
 *4 init_part_thresh (spec, unused)
 *5   "retrieve.parts.thresh"
 *5   "retrieve.parts.length_percent"
 *5   "retrieve.parts.num_match"
 *5   "retrieve.parts.preparse"
 *5   "retrieve.parts.increment"
 *5   "retrieve.vecs_vecs"
 *5   "doc.textloc_file"
 *5   "doc.textloc_file.rmode"
 *5   "retrieve.req.vec_vec.trace"
 *4 close_part_thresh (inst)
 *6 global_context temporarily set to indexing doc (CTXT_DOC)

 *7 Given the two vectors of vec_pair, and an incoming sim, either set
 *7 sim to 0.0 or increment it by increment based upon the similarity of the 
 *7 parts of vector1 and vector2.  Normally used as a restrictive function. 
 *7 For example, perform a normal retrieval between vectors, but then only
 *7 show user those documents whose sentences were sufficiently similar.
 *7 
 *7 The text corresponding to the two vectors (assumed to be both
 *7 documents), given by textloc_file, is preparsed by the procedure
 *7 corresponding to preparse.  Preparse will break the text of the
 *7 document up into appropriate sections, eg sentences.  The procedure
 *7 pp_veclist then constructs a vector for each section.  The two
 *7 vec_list's (lists of vectors for each document) are then pairwise
 *7 compared using the procedure given by vecs_vecs, with all non-zero
 *7 similarities between vectors being reported.  The number of pairs with
 *7 similarity greater than parts.thresh is then counted.  If this number
 *7 is greater than num_match, then sim is left alone, else sim is set to 0.
 *7 
 *7 There is an option to modify the value of parts.thresh used if the docs
 *7 are too short.  For example, if one doc is only one or two concepts,
 *7 then it could be impossible for the similarity to exceed parts.thresh.
 *7 If parts.length_percent is greater than 0.0, then the threshold used is
 *7 MIN (parts.thresh,
 *7      MAX over all vectors in both vec_lists of (parts.length_percent *
 *7          (the sum of the squares of the weights of all terms in vector))).
 *7 
 *7 See req_vec_inv for a sample of a routine that would call part_thresh.

 *9 This is used in current experiments for doc-doc linking, but is not
 *9 very generally useful.
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

static float parts_thresh;          /* Threshold for a match between parts */
static float parts_percent;      /* If > 0.0, then the threshold used will
                                       be MIN (parts_thresh, parts_percent *
                                               perfect match due to longest 
                                               vector)
                                       Needed since for very short docs/queries
                                       may be impossible to obtain 
                                       parts_thresh level */
static long parts_match;
static PROC_INST parts_pre;
static PROC_INST parts_ret;
static char *textloc_file;
static long textloc_mode;
static float increment;

static SPEC_PARAM spec_args[] = {
    "retrieve.parts.thresh",     getspec_float, (char *) &parts_thresh,
    "retrieve.parts.length_percent", getspec_float, (char *) &parts_percent,
    "retrieve.parts.num_match",  getspec_long,  (char *) &parts_match,
    "retrieve.parts.preparse",   getspec_func,  (char *) &parts_pre.ptab,
    "retrieve.parts.increment",  getspec_float, (char *) &increment,
    "retrieve.vecs_vecs",        getspec_func,  (char *) &parts_ret.ptab,
    "doc.textloc_file",             getspec_dbfile,(char *) &textloc_file,
    "doc.textloc_file.rmode",       getspec_filemode, (char *) &textloc_mode,
    TRACE_PARAM ("retrieve.req.vec_vec.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int pp_veclist_inst;
static int textloc_fd;

static int num_inst = 0;

static int get_vec_list();
static void mod_thresh();

static VEC_LIST vec_list1;
static VEC_LIST vec_list2;

int
init_part_thresh (spec, unused)
SPEC *spec;
char *unused;
{
    CONTEXT old_context;

    if (num_inst++ > 0)
        return (num_inst);

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_part_thresh");

    old_context = get_context();
    set_context (CTXT_DOC);
    if (UNDEF == (parts_pre.inst = parts_pre.ptab->init_proc (spec, NULL)) ||
        UNDEF == (parts_ret.inst = parts_ret.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    if (UNDEF == (pp_veclist_inst = init_pp_veclist (spec, (char *) NULL)))
        return (UNDEF);
    set_context (old_context);

    /* Open Textloc file to get file location and section offsets for did */
    if (UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_mode)))
        return (UNDEF);

    /* Initialize cached vec_lists */
    vec_list1.id_num = vec_list2.id_num = UNDEF;

    PRINT_TRACE (8, print_float, &parts_thresh);
    PRINT_TRACE (8, print_long, &parts_match);

    PRINT_TRACE (2, print_string, "Trace: leaving init_part_thresh");
    return (num_inst);
}

int
part_thresh (vec_pair, sim, inst)
VEC_PAIR *vec_pair;
float *sim;
int inst;
{
    VEC_LIST_PAIR vec_list_pair;
    ALT_RESULTS sim_results;
    long num_match;
    long i;
    float thresh_used;    /*  MIN (parts_thresh, parts_percent *
                              perfect match due to longest  vector)
                              Needed since for very short docs/queries
                              may be impossible to obtain parts_thresh level */
    float max_sim;

    PRINT_TRACE (2, print_string, "Trace: entering part_thresh");
    PRINT_TRACE (6, print_long, &vec_pair->vec1->id_num);
    PRINT_TRACE (6, print_long, &vec_pair->vec2->id_num);
    thresh_used = parts_thresh;

    if (vec_pair->vec1->id_num != vec_list1.id_num) {
        if (vec_list1.id_num != UNDEF)
            (void) free_vec_list (&vec_list1);
        if (UNDEF == get_vec_list (vec_pair->vec1->id_num, &vec_list1) ||
            UNDEF == save_vec_list (&vec_list1))
            return (UNDEF);
    }
    vec_list_pair.vec_list1 = &vec_list1;

    (void) mod_thresh (&thresh_used, &parts_percent, &vec_list1);

    if (vec_pair->vec2->id_num != vec_list2.id_num) {
        if (vec_list2.id_num != UNDEF)
            (void) free_vec_list (&vec_list2);
        if (UNDEF == get_vec_list (vec_pair->vec2->id_num, &vec_list2) ||
            UNDEF == save_vec_list (&vec_list2))
            return (UNDEF);
    }
    vec_list_pair.vec_list2 = &vec_list2;

    (void) mod_thresh (&thresh_used, &parts_percent, &vec_list2);

    if (UNDEF == parts_ret.ptab->proc (&vec_list_pair,
                                       &sim_results,
                                       parts_ret.inst))
        return (UNDEF);

    num_match = 0;
    max_sim = 0;
    for (i = 0; i < sim_results.num_results; i++) {
        if (sim_results.results[i].sim >= thresh_used)
            num_match++;
        if (sim_results.results[i].sim >= max_sim)
            max_sim = sim_results.results[i].sim;
            
    }

    PRINT_TRACE (8, print_float, &thresh_used);
    PRINT_TRACE (8, print_float, &max_sim);
    PRINT_TRACE (8, print_long, &num_match);

    if (num_match < parts_match)
        *sim = 0.0;
    else
        *sim += increment;

    PRINT_TRACE (2, print_string, "Trace: leaving part_thresh");
    return (1);
}


int
close_part_thresh (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_part_thresh");

    if (--num_inst) {
        PRINT_TRACE (2, print_string, "Trace: leaving close_part_thresh");
        return (0);
    }

    if (UNDEF == parts_pre.ptab->close_proc (parts_pre.inst) ||
        UNDEF == parts_ret.ptab->close_proc (parts_ret.inst) ||
        UNDEF == close_pp_veclist (pp_veclist_inst) ||
        UNDEF == close_textloc (textloc_fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_part_thresh");
    return (1);
}



static int
get_vec_list (did, vec_list)
long did;
VEC_LIST *vec_list;
{
    TEXTLOC textloc;
    SM_INDEX_TEXTDOC pp;

    textloc.id_num = did;
    pp.id_num = did;
    if (1 != seek_textloc (textloc_fd, &textloc) ||
        1 != read_textloc (textloc_fd, &textloc) ||
        UNDEF == parts_pre.ptab->proc (&textloc, &pp, parts_pre.inst) ||
        UNDEF ==  pp_veclist (&pp, vec_list, pp_veclist_inst))
        return (UNDEF);
    return (1);
}

/* Set *thresh_ptr to 
   MIN (*thresh_ptr,
        MAX (percent_length * sum of squares of terms in vector of vec_list))
*/

static void
mod_thresh (thresh_ptr, percent_length, vec_list)
float *thresh_ptr;
float *percent_length;
VEC_LIST *vec_list;
{
    long i,j;
    float local_sum, max_sum;

    if (*percent_length <= 0.0)
        return;

    max_sum = 0.0;
    for (i = 0; i < vec_list->num_vec && max_sum < *thresh_ptr; i++) {
        local_sum = 0;
        for (j = 0; j < vec_list->vec[i].num_conwt; j++) {
            local_sum += vec_list->vec[i].con_wtp[j].wt *
                         vec_list->vec[i].con_wtp[j].wt;
        }
        local_sum *= *percent_length;
        if (local_sum > max_sum)
            max_sum = local_sum;
    }

    if (max_sum < *thresh_ptr)
        *thresh_ptr = max_sum;
}

