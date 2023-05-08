#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libconvert/pp_partvec.c,v 11.0 1992/07/21 18:22:00 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Index single document to a group of vectors
 *2 pp_partvec (orig_doc, part_vec, inst)
 *3 SM_INDEX_TEXTDOC *orig_doc;
 *3 PART_VEC *part_vec;
 *3 int inst;
 *4 init_pp_partvec (spec, prefix)
 *5   "*.token"
 *5   "*.parse"
 *5   "index.makevec"
 *5   "*.weight"
 *5   "pp_partvec.trace"
 *4 close_pp_partvec (inst)
 *6 context is checked to see if the part specificity has been given
 *6 (eg, CTXT_SENT, CTXT_PARA, CTXT_SECT).  If not, then CTXT_SECT
 *6 is temporarily set (this implies weighting by default will use
 *6 sect_weight).

 *7 Given a preparsed single document text, orig_doc, treat each section
 *7 of orig_doc as a separate document.  Index and weight each section, 
 *7 forming a vector for each.  Then combine all those vectors into
 *7 a part_vec which is sorted by ctype, concept, and then part
 *7 number.

***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "context.h"
#include "section.h"
#include "inst.h"
#include "docdesc.h"
#include "buf.h"
#include "part_vector.h"

static PROC_TAB *makevec;

static SPEC_PARAM spec_args[] = {
    "index.makevec",       getspec_func,      (char *) &makevec,
    TRACE_PARAM ("pp_partvec.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    SM_INDEX_DOCDESC doc_desc;

    /* Instantiation Id's for procedures to be called */
    int *token_inst;
    int *parse_inst;
    int *weight_inst;
    int sectid_inst;
    PROC_INST makevec;

    PART_CON_WT **pconwt;     /* Pool in which PART_CON_WTs are stored,
				 with one per ctype. */
    long *max_num_pconwt;     /* Max Space reserved for conwt */
    long *num_pconwt;         /* Num conwt seen so far for current doc */

} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

#define INIT_ALLOC 4000
#define Malloc(n,type) (type *) malloc( (unsigned) (n)*sizeof(type))
#define Realloc(prev,n,type) (type *) realloc( (char *) (prev), \
					       (unsigned) (n)*sizeof(type))

static int comp_pconwt();

int
init_pp_partvec (spec, passed_prefix)
SPEC *spec;
char *passed_prefix;
{
    int new_inst;
    STATIC_INFO *ip;
    CONTEXT old_context;
    long i;
    char param_prefix[PATH_LEN];
    char *prefix = passed_prefix==NULL ? "" : passed_prefix;

    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_partvec");

    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);

    ip = &info[new_inst];

    if (UNDEF == (lookup_spec_docdesc (spec, &ip->doc_desc)))
        return (UNDEF);

    /* Reserve space for the instantiation ids of the called procedures. */
    if (NULL == (ip->token_inst = (int *)
                malloc ((unsigned) ip->doc_desc.num_sections * sizeof (int)))||
        NULL == (ip->parse_inst = (int *)
                malloc ((unsigned) ip->doc_desc.num_sections * sizeof (int)))||
        NULL == (ip->weight_inst = (int *)
                 malloc ((unsigned) ip->doc_desc.num_ctypes * sizeof (int))))
        return (UNDEF);

    if (NULL==(ip->pconwt = Malloc(ip->doc_desc.num_ctypes,PART_CON_WT *)) ||
	NULL==(ip->max_num_pconwt = Malloc(ip->doc_desc.num_ctypes, long)) ||
	NULL==(ip->num_pconwt = Malloc(ip->doc_desc.num_ctypes, long)))
	return UNDEF;

    for (i=0; i<ip->doc_desc.num_ctypes; i++) {
	ip->max_num_pconwt[i] = INIT_ALLOC;
	if (NULL == (ip->pconwt[i] = Malloc( INIT_ALLOC, PART_CON_WT )))
	    return (UNDEF);
    }

    old_context = get_context();
    if (! check_context (CTXT_SENT) &&
        ! check_context (CTXT_PARA))
        add_context (CTXT_SECT);

    /* Call all initialization procedures */
    for (i = 0; i < ip->doc_desc.num_sections; i++) {
        (void) sprintf (param_prefix, "%sindex.section.%ld.", prefix, i);
        if (UNDEF == (ip->token_inst[i] =
                      ip->doc_desc.sections[i].tokenizer->init_proc
                      (spec, param_prefix)))
            return (UNDEF);
        if (UNDEF == (ip->parse_inst[i] =
                      ip->doc_desc.sections[i].parser->init_proc
                      (spec, param_prefix)))
            return (UNDEF);
    }
    for (i = 0; i < ip->doc_desc.num_ctypes; i++) {
        (void) sprintf (param_prefix, "%sindex.ctype.%ld.", prefix, i);
        if (UNDEF == (ip->weight_inst[i] =
                      ip->doc_desc.ctypes[i].weight_ctype->init_proc
                      (spec, param_prefix)))
            return (UNDEF);
    }

    if (UNDEF == (ip->sectid_inst = init_sectid_num (spec, (char *) NULL)))
        return (UNDEF);

    ip->makevec.ptab = makevec;
    if (UNDEF == (ip->makevec.inst = makevec->init_proc (spec, (char *) NULL)))
        return (UNDEF);

    set_context (old_context);
    ip->valid_info = 1;

    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_partvec");
    return (new_inst);
}

int
pp_partvec (pp_vec, part_vec, inst)
SM_INDEX_TEXTDOC *pp_vec;
PART_VEC *part_vec;
int inst;
{
    SM_TOKENSECT t_sect;
    SM_CONSECT p_sect;
    STATIC_INFO *ip;
    SM_BUF pp_buf;                /* Buffer for text of a single
                                     preparsed section */
    long section_num;
    long i, j, k;
    SM_CONDOC condoc;
    VEC ctype_vec;
    CON_WT *conwt_ptr;
    PART_CON_WT *pconwt_ptr;
    long prev_num_pconwt;
    VEC vec;
    int tot_pconwt;
    int partnums_used = 0;

    PRINT_TRACE (2, print_string, "Trace: entering pp_partvec");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "pp_partvec");
        return (UNDEF);
    }
    ip  = &info[inst];

    for (i=0; i<ip->doc_desc.num_ctypes; i++)
	ip->num_pconwt[i] = 0;

    condoc.num_sections = 1;
    condoc.sections = &p_sect;

    /*
     * Process each section of the document.
     */
    for (i = 0; i < pp_vec->mem_doc.num_sections; i++) {
	short nonempty_vec = 0;

        /* Get the section number corresponding to this section id */
        if (UNDEF == sectid_num (&pp_vec->mem_doc.sections[i].section_id,
                                 &section_num,
                                 ip->sectid_inst))
            return (UNDEF);

        /* Construct a sm_buf giving this section's text, and tokenize it */
        pp_buf.buf = pp_vec->doc_text +
            pp_vec->mem_doc.sections[i].begin_section;
        pp_buf.end = pp_vec->mem_doc.sections[i].end_section -
            pp_vec->mem_doc.sections[i].begin_section;
        t_sect.section_num = section_num;
        if (UNDEF ==
            ip->doc_desc.sections[section_num].tokenizer->proc
                      (&pp_buf,
                       &t_sect,
                       ip->token_inst[section_num]))
            return (UNDEF);

        /* Parse the tokenized section, yielding a list of concept numbers
           and locations */
        if (UNDEF ==
            ip->doc_desc.sections[section_num].parser->proc
                      (&t_sect,
                       &p_sect,
                       ip->parse_inst[section_num]))
            return (UNDEF);

        /* Make a vector out of the section concepts */
        condoc.id_num = i;
        if (UNDEF == ip->makevec.ptab->proc (&condoc, &vec, ip->makevec.inst))
            return (UNDEF);

        /* Reweight the vector, by calling component weighting method
           on each subvector of vec */
        ctype_vec.id_num = vec.id_num;
        ctype_vec.num_ctype = 1;
        conwt_ptr = vec.con_wtp;
        for (j = 0;
             j < vec.num_ctype && conwt_ptr < &vec.con_wtp[vec.num_conwt];
             j++) {
            if (vec.ctype_len[j] <= 0)
                continue;
            ctype_vec.ctype_len = &vec.ctype_len[j];
            ctype_vec.num_conwt = vec.ctype_len[j];
            ctype_vec.con_wtp   = conwt_ptr;
            if (UNDEF == ip->doc_desc.ctypes[j].weight_ctype->
                proc (&ctype_vec, (char *) NULL, ip->weight_inst[j]))
                return (UNDEF);

	    /* Copy this ctype's information into the pool for the ctype */
	    prev_num_pconwt = ip->num_pconwt[j];
	    ip->num_pconwt[j] += ctype_vec.num_conwt;
	    if (ip->num_pconwt[j] > ip->max_num_pconwt[j]) {
		ip->max_num_pconwt[j] += ip->num_pconwt[j];
		if (NULL == (ip->pconwt[j] = Realloc( ip->pconwt[j],
						      ip->max_num_pconwt[j],
						      PART_CON_WT )))
		    return UNDEF;
	    }
	    pconwt_ptr = &ip->pconwt[j][prev_num_pconwt];
	    for (k=0; k<ctype_vec.num_conwt; k++) {
		pconwt_ptr->con = ctype_vec.con_wtp[k].con;
		pconwt_ptr->wt = ctype_vec.con_wtp[k].wt;
		pconwt_ptr->partnum = ctype_vec.id_num;
		pconwt_ptr++;
		nonempty_vec = 1;
	    }

            conwt_ptr += ctype_vec.num_conwt;
        }

	if (nonempty_vec) partnums_used++;
    }

    /*
     * Sort the resulting part vector: part number within concept
     * within concept type.
     */
    tot_pconwt = 0;
    for (i=0; i<ip->doc_desc.num_ctypes; i++) {
	tot_pconwt += ip->num_pconwt[i];

	qsort( (char *) ip->pconwt[i],
	       (int) ip->num_pconwt[i],
	       sizeof( PART_CON_WT ),
	       comp_pconwt );
    }

    /*
     * We want the results to be all strung together in memory,
     * so we put them all after items for ctype 0.  Note that this
     * nicely optimizes the case where there is only one ctype.
     */
    if ( tot_pconwt > ip->max_num_pconwt[0] ) {
	ip->max_num_pconwt[0] = tot_pconwt;
	if (NULL == (ip->pconwt[0] = Realloc( ip->pconwt[0],
					     ip->max_num_pconwt[0],
					     PART_CON_WT )))
	    return UNDEF;
    }
    pconwt_ptr = &ip->pconwt[0][ip->num_pconwt[0]];
    for (i=1; i<ip->doc_desc.num_ctypes; i++) {
	if (ip->num_pconwt[i] > 0)
	    bcopy( (char *) ip->pconwt[i],
		   (char *) pconwt_ptr,
		   (int) ip->num_pconwt[i] * sizeof(PART_CON_WT));
	pconwt_ptr += ip->num_pconwt[i];
    }

    /*
     * Fill in the destination part vector.
     */
    part_vec->id_num = pp_vec->id_num;
    part_vec->max_partnum = pp_vec->mem_doc.num_sections;
    part_vec->partnums_used = partnums_used;
    part_vec->num_ctype = ip->doc_desc.num_ctypes;
    part_vec->num_part_conwt = tot_pconwt;
    part_vec->ctype_len = ip->num_pconwt;
    part_vec->part_con_wtp = ip->pconwt[0];

    PRINT_TRACE (2, print_string, "Trace: leaving pp_partvec");
    return (1);
}


int
close_pp_partvec (inst)
int inst;
{
    STATIC_INFO *ip;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering close_pp_partvec");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "skel");
        return (UNDEF);
    }
    ip  = &info[inst];
    ip->valid_info--;

    if (ip->valid_info == 0) {
        for (i = 0; i < ip->doc_desc.num_sections; i++) {
            if (UNDEF == (ip->doc_desc.sections[i].tokenizer->close_proc
                          (ip->token_inst[i])) ||
                UNDEF == (ip->doc_desc.sections[i].parser->close_proc
                          (ip->parse_inst[i])))
                return (UNDEF);
        }
        for (i = 0; i < ip->doc_desc.num_ctypes; i++) {
            if (UNDEF == (ip->doc_desc.ctypes[i].weight_ctype->close_proc
                          (ip->weight_inst[i])))
                return (UNDEF);
        }
        if (UNDEF == close_sectid_num (ip->sectid_inst))
            return (UNDEF);
        if (UNDEF == ip->makevec.ptab->close_proc (ip->makevec.inst))
            return (UNDEF);

	for (i=0; i<ip->doc_desc.num_ctypes; i++)
	    (void) free( (char *) ip->pconwt[i] );
        (void) free ((char *) ip->pconwt);
        (void) free ((char *) ip->num_pconwt);
        (void) free ((char *) ip->max_num_pconwt);

        (void) free ((char *) ip->token_inst);
        (void) free ((char *) ip->parse_inst);
        (void) free ((char *) ip->weight_inst);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_partvec");
    return (0);
}

static int
comp_pconwt( p1, p2 )
PART_CON_WT *p1, *p2;
{
    if ( p1->con < p2->con )
	return -1;
    if ( p1->con > p2->con )
	return 1;

    if ( p1->partnum < p2->partnum )
	return -1;
    if ( p1->partnum > p2->partnum )
	return 1;

    return 0;
}

