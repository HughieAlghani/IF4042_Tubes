#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libprint/ph_text_fil.c,v 11.0 1992/07/21 18:23:40 chrisb Exp $";
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

static char *filter;

static SPEC_PARAM doc_spec_args[] = {
    "print.indiv.filter",         getspec_string,  (char *) &filter,
    TRACE_PARAM ("print.indiv.trace")
    };
static int num_doc_spec_args = sizeof (doc_spec_args) /
         sizeof (doc_spec_args[0]);

static char *filter_file;

int
init_print_text_filter (spec, unused)
SPEC *spec;
char *unused;
{
    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec,
                              &doc_spec_args[0],
                              num_doc_spec_args)) {
        return (UNDEF);
    }
    PRINT_TRACE (2, print_string, "Trace: entering init_print_text_filter");
    if (VALID_FILE (filter)) {
        if (NULL == (filter_file = malloc (PATH_LEN)))
            return ( UNDEF);
        (void) sprintf (filter_file, "/tmp/sm_fi.%d.o", getpid());
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_print_text_filter");
    return (0);
}

int
print_text_filter (textloc, output, inst)
TEXTLOC *textloc;
SM_BUF *output;
int inst;
{
    TEXTLOC filter_textloc;

    PRINT_TRACE (2, print_string, "Trace: entering print_text_filter");

    if (VALID_FILE (filter)) {
        (void) unlink (filter_file);
        if (-1 == unix_inout_command (filter, textloc->file_name, filter_file))
            return (-1);
        filter_textloc.id_num = textloc->id_num;
        filter_textloc.file_name = filter_file;
        filter_textloc.begin_text = textloc->begin_text;
        filter_textloc.end_text = textloc->end_text;
        print_int_textloc (&filter_textloc, output);
        (void) unlink (filter_file);
    }
    else
        print_int_textloc (textloc, output);

    PRINT_TRACE (2, print_string, "Trace: leaving print_text_filter");
    return (1);
}


int
close_print_text_filter (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_print_text_filter");
    PRINT_TRACE (2, print_string, "Trace: leaving close_print_text_filter");
    return (0);
}

