#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libprint/ph_text_pp.c,v 11.0 1992/07/21 18:23:23 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <fcntl.h>
#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "context.h"
#include "textloc.h"
#include "io.h"
#include "buf.h"

static PROC_INST preparse;

static SPEC_PARAM doc_spec_args[] = {
    "print.doc.preparse",          getspec_func,   (char *) &preparse.ptab,
    TRACE_PARAM ("print.indiv.trace")
    };
static int num_doc_spec_args = sizeof (doc_spec_args) /
         sizeof (doc_spec_args[0]);

static SPEC_PARAM query_spec_args[] = {
    "print.query.preparse",          getspec_func,   (char *) &preparse.ptab,
    TRACE_PARAM ("print.indiv.trace")
    };

static int num_query_spec_args = sizeof (query_spec_args) /
         sizeof (query_spec_args[0]);

int
init_print_text_pp (spec, unused)
SPEC *spec;
char *unused;
{
    /* Lookup the values of the relevant parameters */
    if (check_context (CTXT_DOC)) {
        if (UNDEF == lookup_spec (spec,
                                  &doc_spec_args[0],
                                  num_doc_spec_args)) {
            return (UNDEF);
        }
    }
    else {
        if (UNDEF == lookup_spec (spec,
                                  &query_spec_args[0],
                                  num_query_spec_args)) {
            return (UNDEF);
        }
    }

    PRINT_TRACE (2, print_string, "Trace: entering init_print_text_pp");

    /* Call all initialization procedures */
    if (UNDEF == (preparse.inst = preparse.ptab->init_proc (spec, NULL))) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_print_text_pp");
    return (0);
}

int
print_text_pp (textloc, output, inst)
TEXTLOC *textloc;
SM_BUF *output;
int inst;
{
    SM_INDEX_TEXTDOC pp_doc;

    PRINT_TRACE (2, print_string, "Trace: entering print_text_pp");

    if (1 != preparse.ptab->proc (textloc, &pp_doc, preparse.inst))
        return (UNDEF);
    print_int_textdoc (&pp_doc, output);

    PRINT_TRACE (2, print_string, "Trace: leaving print_text_pp");
    return (1);
}


int
close_print_text_pp (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_print_text_pp");

    if (UNDEF == preparse.ptab->close_proc (preparse.inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_print_text_pp");
    return (0);
}

