#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libobsolete/pp_docsmart.c,v 11.0 1992/07/21 18:23:00 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for indexing procedure description comments of SMART source
 *1 index.preparse.docsmart
 *2 pp_docsmart (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_docsmart (spec_ptr, pp_infile)
 *5   "index.preparse.trace"
 *4 close_pp_docsmart (inst)

 *7 Normal preparser operations on C input files.
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

static PP_SECTIONS  pp_sec_docsmart[] = {
    ".- ",      3,  pp_discard,  '-',  PP_ONELINE|PP_DISPINC, /* Comment */
    ".-\n",     3,  pp_discard,  '-',  PP_ONELINE|PP_DISPINC, /* Comment */
    " *9 ",     4,  pp_copy,     'b',  PP_ONELINE,   /* Bug section */
    " *8 ",     4,  pp_copy,     'a',  PP_ONELINE,   /* Algorithm */
    " *7 ",     4,  pp_copy,     'f',  PP_ONELINE,   /* Full description */
    " *6 ",     4,  pp_copy,     'g',  PP_ONELINE,   /* Global variable */
    " *5 ",     4,  pp_copy,     's',  PP_ONELINE,   /* Spec params or files */
    " *4 ",     4,  pp_copy,     'r',  PP_ONELINE,   /* Related procs */
    " *3 ",     4,  pp_copy,     'p',  PP_ONELINE,   /* Parameters to main  */
    " *2 ",     4,  pp_copy,     'm',  PP_ONELINE,   /* main proc */
    " *1 ",     4,  pp_copy,     'h',  PP_ONELINE,   /* Hierarchy name */
    " *0 ",     4,  pp_copy,     'd',  PP_NEWDOC|PP_ONELINE,  /* Description */
    "",        1,   pp_discard,  '-',  0,
    };

static PP_INFO pp_info = {
    &pp_sec_docsmart[0],
    sizeof (pp_sec_docsmart) / sizeof (pp_sec_docsmart[0]),
    {"", 1, pp_discard, '-', 0},
    PP_TYPE_DOCSMART,
    NULL
    };

static SPEC_PARAM pp[] = {
    TRACE_PARAM ("index.preparse.trace")
    };
static int num_pp = sizeof (pp) / sizeof (pp[0]);

int init_pp_line(), pp_line(), close_pp_line();

int
init_pp_docsmart (spec_ptr, pp_infile)
SPEC *spec_ptr;
char *pp_infile;
{
    int inst;
    if (UNDEF == lookup_spec (spec_ptr, &pp[0], num_pp)) {
        return (UNDEF);
    }
    PRINT_TRACE (2, print_string, "Trace: entering init_pp_docsmart");

    /* initialize the line preparser */
    if (UNDEF == (inst = init_pp_line (&pp_info, pp_infile))) {
        return (UNDEF);
    }
    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_docsmart");

    return (inst);
}

int
pp_docsmart (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{
    int status;
    PRINT_TRACE (2, print_string, "Trace: entering pp_docsmart");

    status = pp_line (input_doc, output_doc, inst);

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_docsmart");

    return (status);
}

int
close_pp_docsmart (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_pp_docsmart");

    if (UNDEF == close_pp_line (inst))
        return (UNDEF);
    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_docsmart");

    return (0);
}


