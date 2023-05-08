#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libconvert/pp_veclist_p.c,v 11.0 1992/07/21 18:22:00 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Index single document to a group of vectors
 *1 convert.tup.pp_veclist_prefix
 *2 pp_veclist_prefix (orig_doc, vec_list, inst)
 *3 SM_INDEX_TEXTDOC *orig_doc;
 *3 VEC_LIST *vec_list;
 *3 int inst;
 *4 init_pp_veclist_prefix (spec, prefix)
 *5   "*.token"
 *5   "*.parse"
 *5   "index.makevec"
 *5   "*.weight"
 *5   "pp_veclist_prefix.trace"
 *4 close_pp_veclist_prefix (inst)
 *6 context is checked to see if the part specificity has been given
 *6 (eg, CTXT_SENT, CTXT_PARA, CTXT_SECT).  If not, then CTXT_SECT
 *6 is temporarily set (this implies weighting by default will use
 *6 sect_weight).
 *7 Given a preparsed single document text, orig_doc, treat each section
 *7 of orig_doc as a separate document.  Index and weight each section, 
 *7 forming a vector for each.  Vec_list contains the list of resulting
 *7 vectors.  Vec_list.id_num gives the id_num of the original document.
 *7 Each vector's id_num gives the corresponding section number of the doc.
 *7
 *7 This is a version of pp_veclist modified only so that a prefix is
 *7 passed into the init procedure.  That prefix is used for finding
 *7 all the subprocedures.

 *9 Warning: if a section ends up not having any indexed terms in it, a
 *9 vector will still exist for it in vec_list, but will have num_conwt set
 *9 to 0.
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

static PROC_TAB *makevec;

static SPEC_PARAM spec_args[] = {
    "index.makevec",       getspec_func,      (char *) &makevec,
    TRACE_PARAM ("pp_veclist_prefix.trace")
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

    CON_WT *conwt;            /* Pool in which all sections CON_WTs
                                 are stored for an individual doc */
    long max_num_conwt;       /* Max Space reserved for conwt */
    long num_conwt;           /* Num conwt seen so far for current doc */

    long *ctype_len;
    long max_num_ctype_len;
    long num_ctype_len;
    
    VEC *veclist;
    long max_num_veclist;
    long num_veclist;
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

#define INIT_ALLOC 4000


int
init_pp_veclist_prefix (spec, passed_prefix)
SPEC *spec;
char *passed_prefix;
{
    int new_inst;
    STATIC_INFO *ip;
    CONTEXT old_context;
    long i;
    char *prefix = passed_prefix == NULL ? "" : passed_prefix;
    char param_prefix[PATH_LEN];

    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_veclist_prefix");

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

    ip->max_num_veclist = 50;
    if (NULL == (ip->veclist = (VEC *)
                 malloc ((unsigned) ip->max_num_veclist * sizeof (VEC))))
        return (UNDEF);

    ip->max_num_conwt = INIT_ALLOC;
    if (NULL == (ip->conwt = (CON_WT *)
                 malloc ((unsigned) ip->max_num_conwt * sizeof (CON_WT))))
        return (UNDEF);

    ip->max_num_ctype_len = INIT_ALLOC;
    if (NULL == (ip->ctype_len = (long *)
		 malloc ((unsigned)ip->max_num_ctype_len * sizeof (long))))
	return (UNDEF);

    old_context = get_context();
    if (! check_context (CTXT_SENT) &&
        ! check_context (CTXT_PARA))
        add_context (CTXT_SECT);

    /* Call all initialization procedures */
    for (i = 0; i < ip->doc_desc.num_sections; i++) {
        (void) sprintf (param_prefix, "%sindex.section.%ld.",
			prefix, i);
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

    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_veclist_prefix");
    return (new_inst);
}

int
pp_veclist_prefix (pp_vec, vec_list, inst)
SM_INDEX_TEXTDOC *pp_vec;
VEC_LIST *vec_list;
int inst;
{
    SM_TOKENSECT t_sect;
    SM_CONSECT p_sect;
    STATIC_INFO *ip;
    SM_BUF pp_buf;                /* Buffer for text of a single
                                     preparsed section */
    long section_num;
    long i, j;
    SM_CONDOC condoc;
    VEC ctype_vec;
    CON_WT *conwt_ptr;
    VEC vec;

    PRINT_TRACE (2, print_string, "Trace: entering pp_veclist_prefix");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "pp_veclist_prefix");
        return (UNDEF);
    }
    ip  = &info[inst];

    ip->num_conwt = 0;
    ip->num_veclist = 0;
    ip->num_ctype_len = 0;

    condoc.num_sections = 1;
    condoc.sections = &p_sect;

    /* Reserve space for vector list */
    if (pp_vec->mem_doc.num_sections >= ip->max_num_veclist) {
        ip->max_num_veclist += pp_vec->mem_doc.num_sections;
        if (NULL == (ip->veclist = (VEC *)
                     realloc ((char *) ip->veclist,
                              (unsigned) ip->max_num_veclist *
                              sizeof (VEC))))
            return (UNDEF);
    }
    /* Reserve space for ctypes */
    if (pp_vec->mem_doc.num_sections * ip->doc_desc.num_ctypes >
        ip->max_num_ctype_len) {
        if (ip->max_num_ctype_len != 0)
            (void) free ((char *)ip->ctype_len);
        ip->max_num_ctype_len += pp_vec->mem_doc.num_sections *
            ip->doc_desc.num_ctypes;
        if (NULL == (ip->ctype_len = (long *)
                     malloc ((unsigned)ip->max_num_ctype_len * sizeof (long))))
            return (UNDEF);
    }

    for (i = 0; i < pp_vec->mem_doc.num_sections; i++) {
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
            conwt_ptr += ctype_vec.num_conwt;
        }

        /* Copy the vector into the CON_WT/ctype global pools */
        if (ip->num_conwt + vec.num_conwt >= ip->max_num_conwt) {
            ip->max_num_conwt += ip->num_conwt + vec.num_conwt;
            if (NULL == (ip->conwt = (CON_WT *)
                         realloc ((char *) ip->conwt,
                                  (unsigned) ip->max_num_conwt *
                                  sizeof (CON_WT))))
                return (UNDEF);
        }
        (void) bcopy ((char *) vec.con_wtp,
                      (char *) &ip->conwt[ip->num_conwt],
                      (int) vec.num_conwt * sizeof (CON_WT));
        (void) bcopy ((char *) vec.ctype_len,
                      (char *) &ip->ctype_len[ip->num_ctype_len],
                      (int) vec.num_ctype * sizeof (long));
	ip->veclist[ip->num_veclist].id_num = vec.id_num;
        ip->veclist[ip->num_veclist].num_conwt = vec.num_conwt;
        ip->veclist[ip->num_veclist].num_ctype = vec.num_ctype;
        ip->veclist[ip->num_veclist].ctype_len =
            &ip->ctype_len[ip->num_ctype_len];
        ip->num_conwt += vec.num_conwt;
        ip->num_ctype_len += vec.num_ctype;
        ip->num_veclist++;
    }

    /* Must go back and fill in the con_wt pointers into the global
       conwt array (have to do now in case realloc moved it). */
    conwt_ptr = ip->conwt;
    for (i = 0; i < ip->num_veclist; i++) {
        ip->veclist[i].con_wtp = conwt_ptr;
        conwt_ptr += ip->veclist[i].num_conwt;
    }

    vec_list->num_vec = ip->num_veclist;
    vec_list->vec = ip->veclist;
    vec_list->id_num = pp_vec->id_num;

    PRINT_TRACE (4, print_vec_list, vec_list);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_veclist_prefix");
    return (1);
}


int
close_pp_veclist_prefix (inst)
int inst;
{
    STATIC_INFO *ip;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering close_pp_veclist_prefix");

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

        (void) free ((char *) ip->conwt);
        (void) free ((char *) ip->ctype_len);
        (void) free ((char *) ip->veclist);
        (void) free ((char *) ip->token_inst);
        (void) free ((char *) ip->parse_inst);
        (void) free ((char *) ip->weight_inst);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_veclist_prefix");
    return (0);
}
