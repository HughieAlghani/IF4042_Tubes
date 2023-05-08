#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/pp_text.c,v 11.0 1992/07/21 18:23:02 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for simple documents (one doc per file, all text)
 *1 index.preparse.text
 *2 pp_text (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_text (spec_ptr, unused)
 *5   "index.preparse.trace"
 *4 close_pp_text (inst)
 *6 Uses global_context to tell if indexing doc or query (CTXT_DOC, CTXT_QUERY)

 *7 Normal preparser operations on collection which is one document per file,
 *7 and all text should be included in a singel section.
 *7 Puts a preparsed document in output_doc which corresponds to either
 *7 the input_doc (if non-NULL), or the next document found from the list of
 *7 documents in pp_infile
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error

 *8 Sets up preparse description array corresponding to what is wanted
 *8 for smart, and gives it to pp_line procedures which do all the work.  
***********************************************************************/

#include "common.h"
#include "param.h"
#include "spec.h"
#include "preparser.h"
#include "smart_error.h"
#include "functions.h"
#include "docindex.h"
#include "trace.h"
#include "context.h"

static PP_SECTIONS  pp_sec_text[] = {
    "",        1,  pp_copy,     'w',  0,
    };

static PP_INFO pp_info = {
    &pp_sec_text[0],
    sizeof (pp_sec_text) / sizeof (pp_sec_text[0]),
    {"", 1, pp_copy, 'w', 0},
    PP_TYPE_TEXT,
    NULL
    };


static SPEC_PARAM pp[] = {
    TRACE_PARAM ("index.preparse.trace")
    };
static int num_pp = sizeof (pp) / sizeof (pp[0]);

int init_pp_line(), pp_line(), close_pp_line();

int
init_pp_text (spec_ptr, pp_infile)
SPEC *spec_ptr;
char *pp_infile;
{
    int inst;
    if (UNDEF == lookup_spec (spec_ptr, &pp[0], num_pp))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_text");

    /* initialize the line preparser */
    if (UNDEF == (inst = init_pp_line (&pp_info, pp_infile)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_text");

    return (inst);
}

int
pp_text (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{
    int status;
    PRINT_TRACE (2, print_string, "Trace: entering pp_text");

    status = pp_line (input_doc, output_doc, inst);

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_text");

    return (status);
}

int
close_pp_text (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_pp_text");

    if (UNDEF == close_pp_line (inst))
        return (UNDEF);
    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_text");

    return (0);
}


