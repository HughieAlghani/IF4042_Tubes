#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libindexing/pp_smart_nc.c,v 11.0 1992/07/21 18:22:10 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for documents in old standard smart format
 *1 index.preparse.smart_nc
 *2 pp_smart_nc (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_smart_nc (spec_ptr, unused)
 *5   "index.preparse.trace"
 *4 close_pp_smart_nc (inst)
 *6 Uses global_context to tell if indexing doc or query (CTXT_DOC, CTXT_QUERY)

 *7 Normal preparser operations on standard SMART experimental collections.
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

static PP_SECTIONS  pp_sec_smart[] = {
    ".x ",     3,  pp_copy,     'x',  0,
    ".x\n",    3,  pp_copy,     'x',  0,
    ".w ",     3,  pp_copy,     'w',  0,
    ".w\n",    3,  pp_copy,     'w',  0,
    ".t ",     3,  pp_copy,     't',  0,
    ".t\n",    3,  pp_copy,     't',  0,
    ".o ",     3,  pp_copy,     'o',  0,
    ".o\n",    3,  pp_copy,     'o',  0,
    ".n ",     3,  pp_copy,     'n',  0,
    ".n\n",    3,  pp_copy,     'n',  0,
    ".k ",     3,  pp_copy,     'k',  0,
    ".k\n",    3,  pp_copy,     'k',  0,
    ".i",      2,  pp_discard,  '-',  PP_NEWDOC|PP_ONELINE,
    ".c ",     3,  pp_copy,     'c',  0,
    ".c\n",    3,  pp_copy,     'c',  0,
    ".b ",     3,  pp_copy,     'b',  0,
    ".b\n",    3,  pp_copy,     'b',  0,
    ".a ",     3,  pp_copy,     'a',  0,
    ".a\n",    3,  pp_copy,     'a',  0,
    ".X ",     3,  pp_copy,     'x',  0,
    ".X\n",    3,  pp_copy,     'x',  0,
    ".W ",     3,  pp_copy,     'w',  0,
    ".W\n",    3,  pp_copy,     'w',  0,
    ".T ",     3,  pp_copy,     't',  0,
    ".T\n",    3,  pp_copy,     't',  0,
    ".O ",     3,  pp_copy,     'o',  0,
    ".O\n",    3,  pp_copy,     'o',  0,
    ".N ",     3,  pp_copy,     'n',  0,
    ".N\n",    3,  pp_copy,     'n',  0,
    ".K ",     3,  pp_copy,     'k',  0,
    ".K\n",    3,  pp_copy,     'k',  0,
    ".I",      2,  pp_discard,  '-',  PP_NEWDOC|PP_ONELINE,
    ".C ",     3,  pp_copy,     'c',  0,
    ".C\n",    3,  pp_copy,     'c',  0,
    ".B ",     3,  pp_copy,     'b',  0,
    ".B\n",    3,  pp_copy,     'b',  0,
    ".A ",     3,  pp_copy,     'a',  0,
    ".A\n",    3,  pp_copy,     'a',  0,
    ".- ",     3,  pp_discard,  '-',  0,
    ".-\n",    3,  pp_discard,  '-',  0,
    "",        0,  pp_copy,     'w',  0,  /* Sentinel */
    };

static PP_INFO pp_info = {
    &pp_sec_smart[0],
    sizeof (pp_sec_smart) / sizeof (pp_sec_smart[0]),
    {"", 0, pp_copy, 'w', 0},
    PP_TYPE_SMART,
    NULL
    };


static SPEC_PARAM pp[] = {
    TRACE_PARAM ("index.preparse.trace")
    };
static int num_pp = sizeof (pp) / sizeof (pp[0]);

int init_pp_line_nc(), pp_line_nc(), close_pp_line_nc();

int
init_pp_smart_nc (spec_ptr, pp_infile)
SPEC *spec_ptr;
char *pp_infile;
{
    int inst;
    if (UNDEF == lookup_spec (spec_ptr, &pp[0], num_pp))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_smart_nc");

    /* initialize the line preparser */
    if (UNDEF == (inst = init_pp_line_nc (&pp_info, pp_infile)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_smart_nc");

    return (inst);
}

int
pp_smart_nc (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{
    int status;
    PRINT_TRACE (2, print_string, "Trace: entering pp_smart_nc");

    status = pp_line_nc (input_doc, output_doc, inst);

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_smart_nc");

    return (status);
}

int
close_pp_smart_nc (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_pp_smart_nc");

    if (UNDEF == close_pp_line_nc (inst))
        return (UNDEF);
    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_smart_nc");

    return (0);
}


