#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/pp_parts.c,v 11.0 1992/07/21 18:21:24 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser front-ends that break sections at appropriate boundaries
 *2 pp_parts (input_doc, out_pp, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_parts (spec_ptr, pp_infile)
 *5   "index.parts_preparse.trace"
 *5   "pp_parts.preparse"
 *4 close_pp_parts (inst)

 *7 Puts a preparsed document in output_doc which corresponds to either
 *7 the input_doc (if non-NULL), or the next document found from the list of
 *7 documents in pp_infile
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error

 *8 Calls the pp_parts.preparse preparser to actually preparse input_doc,
 *8 and then calls pp_pp_parts to convert the original document into one
 *8 where sections have been defined at the appropriate boundaries.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "spec.h"
#include "trace.h"
#include "io.h"
#include "inst.h"
#include "proc.h"
#include "sm_display.h"
#include "docindex.h"
#include "context.h"
#include "section.h"

static PROC_TAB *pp;        /* Preparsing procedure */

static SPEC_PARAM spec_args[] = {
    "pp_parts.preparse",         getspec_func,    (char *) &pp,
    TRACE_PARAM ("index.parts_preparse.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);


/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    PROC_INST pp;
    int pp_pp_parts_inst;
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

static int init_pp_parts();

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser front-end that breaks sections at sentence boundaries
 *1 index.parts_preparse.sent
 *2 pp_parts (input_doc, out_pp, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_sent (spec_ptr, pp_infile)
 *5   "index.parts_preparse.trace"
 *5   "pp_parts.preparse"
 *4 close_pp_parts (inst)

 *6 Uses global_context to set sentence preparse action (CTXT_SENT)

 *7 Puts a preparsed document in output_doc which corresponds to either
 *7 the input_doc (if non-NULL), or the next document found from the list of
 *7 documents in doc_loc (or query_loc if indexing query)
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error

 *8 Calls the pp_parts.preparse preparser to actually preparse input_doc,
 *8 and then calls pp_pp_parts to convert the original document into one
 *8 where sections have been defined at the appropriate boundaries.
***********************************************************************/

int
init_pp_sent (spec, pp_infile)
SPEC *spec;
char *pp_infile;
{
    CONTEXT old_context;
    int inst;

    old_context = get_context();
    add_context (CTXT_SENT);

    if (UNDEF == (inst = init_pp_parts (spec, pp_infile)))
        return (UNDEF);
    
    set_context (old_context);

    return (inst);
}

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser front-end that breaks sections at paragraph boundaries
 *1 index.parts_preparse.para
 *2 pp_parts (input_doc, out_pp, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_para (spec_ptr, pp_infile)
 *5   "index.parts_preparse.trace"
 *5   "pp_parts.preparse"
 *4 close_pp_parts (inst)

 *6 Uses global_context to set paragraph preparse action (CTXT_PARA)

 *7 Puts a preparsed document in output_doc which corresponds to either
 *7 the input_doc (if non-NULL), or the next document found from the list of
 *7 documents in pp_infile.
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error

 *8 Calls the pp_parts.preparse preparser to actually preparse input_doc,
 *8 and then calls pp_pp_parts to convert the original document into one
 *8 where sections have been defined at the appropriate boundaries.
***********************************************************************/

int
init_pp_para (spec, pp_infile)
SPEC *spec;
char *pp_infile;
{
    CONTEXT old_context;
    int inst;

    old_context = get_context();
    add_context (CTXT_PARA);

    if (UNDEF == (inst = init_pp_parts (spec, pp_infile)))
        return (UNDEF);
    
    set_context (old_context);

    return (inst);
}

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser front-end that breaks pp sections at doc section boundaries
 *1 index.parts_preparse.sect
 *2 pp_parts (input_doc, out_pp, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_sect (spec_ptr, pp_infile)
 *5   "index.parts_preparse.trace"
 *5   "pp_parts.preparse"
 *4 close_pp_parts (inst)

 *6 Uses global_context to set section preparse action (CTXT_SECT)

 *7 Puts a preparsed document in output_doc which corresponds to either
 *7 the input_doc (if non-NULL), or the next document found from the list of
 *7 documents in pp_infile
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error

 *8 Calls the pp_parts.preparse preparser to actually preparse input_doc,
 *8 and then calls pp_pp_parts to convert the original document into one
 *8 where sections have been defined at the appropriate boundaries.
***********************************************************************/

int
init_pp_sect (spec, pp_infile)
SPEC *spec;
char *pp_infile;
{
    CONTEXT old_context;
    int inst;

    old_context = get_context();
    add_context (CTXT_SECT);

    if (UNDEF == (inst = init_pp_parts (spec, pp_infile)))
        return (UNDEF);
    
    set_context (old_context);

    return (inst);
}

static int
init_pp_parts (spec, pp_infile)
SPEC *spec;
char *pp_infile;
{
    STATIC_INFO *ip;
    int new_inst;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_parts");
    
    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);
    
    ip = &info[new_inst];

    /* Call all initialization procedures for indexing */
    ip->pp.ptab = pp;
    if (UNDEF == (ip->pp.inst = pp->init_proc (spec, pp_infile)))
        return (UNDEF);
    if (UNDEF == (ip->pp_pp_parts_inst =
                  init_pp_pp_parts (spec, (char *)NULL)))
        return (UNDEF);

    ip->valid_info = 1;
             
    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_parts");

    return (new_inst);
}

int
pp_parts (input_doc, out_pp, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *out_pp;
int inst;
{
    STATIC_INFO *ip;
    SM_INDEX_TEXTDOC orig_pp;

    PRINT_TRACE (2, print_string, "Trace: entering pp_parts");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "pp_parts");
        return (UNDEF);
    }
    ip  = &info[inst];

    if (UNDEF == ip->pp.ptab->proc (input_doc, &orig_pp, ip->pp.inst) ||
        UNDEF == pp_pp_parts (&orig_pp, out_pp, ip->pp_pp_parts_inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving pp_parts");

    return (1);
}

int
close_pp_parts(inst)
int inst;
{
    STATIC_INFO *ip;

    PRINT_TRACE (2, print_string, "Trace: entering close_pp_parts");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_pp_parts");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;

    if (UNDEF == ip->pp.ptab->close_proc (ip->pp.inst) ||
        UNDEF == close_pp_pp_parts (ip->pp_pp_parts_inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_parts");
    return (0);
}

