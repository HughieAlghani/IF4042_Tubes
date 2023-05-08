#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/./src/libindexing/pp_trec.c,
v 10.1 91/11/05 23:49:33 smart Exp Locker: smart $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley.

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for trec queries
 *1 index.preparse.trec_query
 *2 pp_trec_query (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_trec_query (spec_ptr, pp_infile)
 *5   "index.preparse.trace"
 *4 close_pp_trec_query (inst)
 *6 Uses global_context to tell if indexing doc or query (CTXT_DOC, CTXT_QUERY)

 *7 Normal preparser operations on trec_query collection.
 *7 Should be rewritten to parse header fields separately, and to explicitly
 *7 try and recognize signatures (recognizes some that are added automatically
 *7 by normal trec_query software)
 *7 Recognizes many (but not all) header fields from many trec_query sub-collections.
 *7 Puts a preparsed document in output_doc which corresponds to either
 *7 the input_doc (if non-NULL), or the next document found from the list of
 *7 documents in pp_infile.
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error

 *8 Sets up preparse description array corresponding to what is wanted
 *8 for trec_query, and gives it to pp_line_nc procedures which do all the work.
***********************************************************************/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "spec.h"
#include "preparser.h"
#include "smart_error.h"
#include "functions.h"
#include "docindex.h"
#include "trace.h"
#include "context.h"

static PP_SECTIONS  pp_sec_trec_query[] = {
    "<top>",               5,    pp_discard,   '-',  PP_NEWDOC,
    "<title> Topic:",      14,   pp_copy,      't',  PP_DISPINC,
    "<time>",              6,    pp_discard,   '-',  PP_DISPINC,
    "<num>",               5,    pp_copy,      'n',  PP_DISPINC,
    "<nat> Nationality:",  18,   pp_copy,      'w',  PP_DISPINC,
    "<narr> Narrative:",   17,   pp_copy,      'w',  PP_DISPINC,
    "<head>",              6,    pp_discard,   '-',  PP_DISPINC,
    "<fac> Factor(s):",    16,   pp_copy,      'w',  PP_DISPINC,
    "<dom>",               5,    pp_discard,   '-',  PP_DISPINC,
    "<desc> Description:", 19,   pp_copy,      't',  PP_DISPINC,
    "<def> Definition(s):",20,   pp_discard,   '-',  PP_DISPINC,
    "<con> Concept(s):",   17,   pp_copy,      'w',  PP_DISPINC,
    "</top>",              6,    pp_discard,   '-',  PP_DISPINC,
    "</fac>",              6,    pp_discard,   '-',  PP_DISPINC,
           /* Note: guaranteed no  match */
    "",                 1,  pp_copy,     '-',  PP_ONELINE | PP_DISPINC
    };

static PP_INFO pp_info = {
    &pp_sec_trec_query[0],
    sizeof (pp_sec_trec_query) / sizeof (pp_sec_trec_query[0]),
    {"", 0, pp_copy, 'w', 0},
    PP_TYPE_TREC,
    NULL
    };


static SPEC_PARAM pp[] = {
    TRACE_PARAM ("index.preparse.trace")
    };
static int num_pp = sizeof (pp) / sizeof (pp[0]);

int init_pp_line(), pp_line(), close_pp_line();

int
init_pp_trec_query (spec_ptr, pp_infile)
SPEC *spec_ptr;
char *pp_infile;
{
    int inst;

    if (UNDEF == lookup_spec (spec_ptr, &pp[0], num_pp))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_pp_trec_query");

    /* initialize the line preparser */
    if (UNDEF == (inst = init_pp_line (&pp_info, pp_infile)))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_trec_query");

    return (inst);
}

int
pp_trec_query (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{
    int status;

    PRINT_TRACE (2, print_string, "Trace: entering pp_trec_query");

    status = pp_line (input_doc, output_doc, inst);

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_trec_query");

    return (status);
}

int
close_pp_trec_query (inst)
int inst;
{
    int status;
    PRINT_TRACE (2, print_string, "Trace: entering close_pp_trec_query");

    status = close_pp_line (inst);
    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_trec_query");

    return (status);
}
