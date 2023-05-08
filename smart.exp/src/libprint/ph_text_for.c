#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libprint/ph_text_for.c,v 11.0 1992/07/21 18:23:23 chrisb Exp $";
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
static char *format;

static SPEC_PARAM doc_spec_args[] = {
    "print.doc.preparse",          getspec_func,   (char *) &preparse.ptab,
    "print.doc.format",            getspec_string, (char *) &format,
    TRACE_PARAM ("print.indiv.trace")
    };
static int num_doc_spec_args = sizeof (doc_spec_args) /
         sizeof (doc_spec_args[0]);

static SPEC_PARAM query_spec_args[] = {
    "print.query.preparse",          getspec_func,   (char *) &preparse.ptab,
    "print.query.format",            getspec_string, (char *) &format,
    TRACE_PARAM ("print.indiv.trace")
    };

static int num_query_spec_args = sizeof (query_spec_args) /
         sizeof (query_spec_args[0]);

static SM_INDEX_DOCDESC doc_desc;    /* New style doc description */

static spec_id = 0;              /* Id for spec last time print_text_form
                                    was called.  If unchanged, no need
                                    to re-initialize */

static void print_textdoc_form();

int
init_print_text_form (spec, unused)
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

    PRINT_TRACE (2, print_string, "Trace: entering init_print_text_form");

    if (spec->spec_id != spec_id &&
        UNDEF == lookup_spec_docdesc (spec, &doc_desc)) {
        return (UNDEF);
    }
    spec_id = spec->spec_id;
    
    /* Call all initialization procedures */
    if (UNDEF == (preparse.inst = preparse.ptab->init_proc (spec, NULL))) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_print_text_form");
    return (0);
}

int
print_text_form (textloc, output, inst)
TEXTLOC *textloc;
SM_BUF *output;
int inst;
{
    SM_INDEX_TEXTDOC pp_doc;

    PRINT_TRACE (2, print_string, "Trace: entering print_text_form");
    pp_doc.id_num = textloc->id_num;
    if (1 != preparse.ptab->proc (textloc, &pp_doc, preparse.inst))
        return (UNDEF);
    print_textdoc_form (&pp_doc, output);

    PRINT_TRACE (2, print_string, "Trace: leaving print_text_form");
    return (1);
}


int
close_print_text_form (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_print_text_form");

    if (UNDEF == preparse.ptab->close_proc (preparse.inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_print_text_form");
    return (0);
}

static SM_BUF internal_output = {0, 0, (char *) 0};

static void
print_textdoc_form (doc, output)
SM_INDEX_TEXTDOC *doc;
SM_BUF *output;
{
    SM_BUF *out_p;
    char *ptr;
    long i;
    long section_length;
    int nl_to_sp;             /* Flag to indicate want to compress \n to ' ' */
    SM_BUF sm_buf;
    char buf;
    int save_start;
    char *temp_ptr;

    if (! VALID_FILE (format)) {
        print_int_textdoc (doc, output);
        return;
    }

    if (doc->mem_doc.num_sections == 0)
        return;

    if (output == NULL) {
        out_p = &internal_output;
        out_p->end = 0;
    }
    else {
        out_p = output;
    }

    for (ptr = format; *ptr; ptr++) {
        switch (*ptr) {
          case '\\':
            sm_buf.buf = &buf;
            sm_buf.end = 1;
            ptr++;
            switch (*ptr) {
              case 'n':
                buf = '\n';
                break;
              case 't':
                buf = '\t';
                break;
              case 'b':
                buf = '\b';
                break;
              case 'r':
                buf = '\r';
                break;
              case 'f':
                buf = '\f';
                break;
              default:    /* BUG?? Does not handle octal numbers */
                buf = *ptr;
                break;
            }
            if (UNDEF == add_buf (&sm_buf, output))
                return;
            break;
          case '%':
            ptr++;
            if (*ptr == '-') {
                nl_to_sp = 1;
                ptr++;
            }
            else {
                nl_to_sp = 0;
            }
            for (i = 0; i < doc->mem_doc.num_sections; i++) {
                if (*ptr == doc->mem_doc.sections[i].section_id) {
                    section_length = doc->mem_doc.sections[i].end_section -
                                     doc->mem_doc.sections[i].begin_section;

                    sm_buf.buf = doc->doc_text +
                                  doc->mem_doc.sections[i].begin_section;
                    sm_buf.end = section_length;
                    save_start = output->end;
                    if (UNDEF == add_buf (&sm_buf, output))
                        return;
                    if (nl_to_sp) {
                        /* Change all newlines to spaces */
                        for (temp_ptr = &output->buf[save_start];
                             temp_ptr < &output->buf[output->end];
                             temp_ptr++) {
                            if (*temp_ptr == '\n')
                                *temp_ptr = ' ';
                        }
                    }
                }
            }
            break;
          default:
            sm_buf.buf = &buf;
            sm_buf.end = 1;
            buf = *ptr;
            if (UNDEF == add_buf (&sm_buf, output))
                return;
            break;
        }
    }
    if (output == NULL) {
        (void) fwrite (out_p->buf, 1, out_p->end, stdout);
        (void) fflush (stdout);
        out_p->end = 0;
    }
}
