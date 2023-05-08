#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/liblocal/libindexing/pp_expnews.c,v 11.0 1992/07/21 18:22:09 chrisb Exp $";
#endif
/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 pre-parser for a particular Cornell version of news collection
 *1 index.preparse.expnews
 *2 pp_expnews (input_doc, output_doc, inst)
 *3   TEXTLOC *input_doc;
 *3   SM_INDEX_TEXTDOC *output_doc;
 *3   int inst;
 *4 init_pp_expnews (spec_ptr, pp_infile)
 *5   "index.preparse.trace"
 *4 close_pp_expnews (inst)
 *6 Uses global_context to tell if indexing doc or query (CTXT_DOC, CTXT_QUERY)
 *7 Normal preparser operations on news, plus handles quoted material
 *7 separately from normal material.  
 *7 Puts a preparsed document in output_doc which corresponds to either
 *7 the input_doc (if non-NULL), or the next document found from the list of
 *7 documents in pp_infile
 *7 Returns 1 if found doc to preparse, 0 if no more docs, UNDEF if error
 *8 Sets up preparse description array corresponding to what is wanted
 *8 for this version of news, and gives it to pp_line procedures which do
 *8 all the work.
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


#ifdef NOTDEF
static char *pp_copy_a();
#endif
static char *pp_copy_q();
extern void pp_putchar();

static PP_SECTIONS  pp_sec_news[] = {
    "|",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "Xref:",            5,  pp_copy,     'r',  PP_ONELINE | PP_DISPINC,
    "Title:",           6,  pp_copy,     's',  PP_ONELINE | PP_DISPINC,
    "Summary:",         8,  pp_copy,     's',  PP_ONELINE | PP_DISPINC,
    "Subject:",         8,  pp_copy,     's',  PP_ONELINE | PP_DISPINC,
    "Sender:",          7,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Reply-To:",        9,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Relay-Version:",   14, pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "References:",      11, pp_copy,     'r',  PP_ONELINE | PP_DISPINC,
    "Posting-Version:", 16, pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Posted:",          7,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Path:",            5,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Organization:",    13, pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Newsgroups:",      11, pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Message-ID:",      11, pp_copy,     'r',  PP_ONELINE | PP_DISPINC,
    "Lines:",           6,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Keywords:",        9,  pp_copy,     's',  PP_ONELINE | PP_DISPINC,
    "In-reply-to:",     12, pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "In article <",     12, pp_copy_q,   'q',  PP_ONELINE | PP_DISPINC,
    "In article  <",    13, pp_copy_q,   'q',  PP_ONELINE | PP_DISPINC,
    "From:",            5,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Followup-To:",     12, pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Expires:",         8,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Document path:",   14, pp_discard,  '-',  PP_ONELINE|PP_DISPINC|PP_NEWDOC,
    "Date:",            5,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Date-Received:",   14, pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "Article-I.D.:",    13, pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    ">In article <",    13, pp_copy_q,   'q',  PP_ONELINE | PP_DISPINC,
    ">>",               2,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    ">",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "=",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "<",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "/",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    ".- ",              3,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    ".-\n",             3,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "---------",        9,  pp_discard,  '-',  PP_ONELINE | PP_DISPINC,
    "---",              3,  pp_copy,     'w',  PP_DISPINC,
    "-- \n",            4,  pp_discard,  '-',  PP_DISPINC,
    "--\n",             3,  pp_discard,  '-',  PP_DISPINC,
    "-",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "+",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "%",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
    "$",                1,  pp_copy,     'q',  PP_ONELINE | PP_DISPINC,
           /* Note: guaranteed no  match */
    "",                 1,  pp_copy,     'w',  PP_ONELINE | PP_DISPINC
    };

static PP_INFO pp_info = {
    &pp_sec_news[0],
    sizeof (pp_sec_news) / sizeof (pp_sec_news[0]),
    {"", 1, pp_copy, 'w', 0},
    PP_TYPE_EXPNEWS,
    NULL
    };

static SPEC_PARAM pp[] = {
    TRACE_PARAM ("index.preparse.trace")
    };
static int num_pp = sizeof (pp) / sizeof (pp[0]);

int init_pp_line(), pp_line(), close_pp_line();

int
init_pp_expnews (spec_ptr, pp_infile)
SPEC *spec_ptr;
char *pp_infile;
{
    int inst;
    if (UNDEF == lookup_spec (spec_ptr, &pp[0], num_pp)) {
        return (UNDEF);
    }
    PRINT_TRACE (2, print_string, "Trace: entering init_pp_expnews");

    /* initialize the line preparser */
    if (UNDEF == (inst = init_pp_line (&pp_info, pp_infile))) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving init_pp_expnews");

    return (inst);
}

int
pp_expnews (input_doc, output_doc, inst)
TEXTLOC *input_doc;
SM_INDEX_TEXTDOC *output_doc;
int inst;
{
    int status;

    PRINT_TRACE (2, print_string, "Trace: entering pp_expnews");

    status = pp_line (input_doc, output_doc, inst);

    PRINT_TRACE (4, print_int_textdoc, output_doc);
    PRINT_TRACE (2, print_string, "Trace: leaving pp_expnews");

    return (status);
}

int
close_pp_expnews (inst)
int inst;
{
    int status;
    PRINT_TRACE (2, print_string, "Trace: entering close_pp_expnews");

    status = close_pp_line (inst);
    PRINT_TRACE (2, print_string, "Trace: leaving close_pp_expnews");

    return (status);
}


/* In lines of the form "In article <id> person writes:" we want to  */
/* eliminate the "writes:" */
static char *
pp_copy_q (start_ptr, output_ptr)
char *start_ptr;
char **output_ptr;     
{
    register char *ptr = start_ptr;
    int article_found, write_found;
    char temp_buf[PATH_LEN];

    article_found = write_found = 0;
    while (*ptr != '\n') {
        if (*ptr == 'a' && 0 == strncmp (ptr, "article", 7))
            article_found = 1;
        if (article_found && *ptr == 'w' &&
            0 == strncmp (ptr, "writes:\n", 8)) {
            (void) strncpy (temp_buf,
                            start_ptr,
                            MIN ((ptr-start_ptr), (PATH_LEN-1)));
            temp_buf[MIN ((ptr-start_ptr), (PATH_LEN-1))] = '\0';
            write_found = 1;
        }
    }

    (void) pp_copy (write_found ? temp_buf : start_ptr, output_ptr);

    return (ptr+1);
}

#ifdef NOTDEF
/* Eliminate the components of net addresses by removing '.', '@', '!'
   from text when surrounded by non-space characters */
static char *
pp_copy_a (ptr)
register char *ptr;
{
    char *start = ptr;

    while (*ptr) {
        if (*ptr <= '@' &&
            (*ptr == '.' || *ptr == '@' || *ptr == '!') &&
            (! isspace (*(ptr+1))) &&
            (ptr != start) &&
            (! isspace (*(ptr-1)))) {
            ptr++;
        }
        /* Do not output non-ascii characters (the parser objects) or */
        /* control-characters */
        if (isascii (*ptr) && (isprint (*ptr) || isspace (*ptr))) {
            pp_putchar (*ptr);
        }
        ptr++;
    }
    return (ptr);
}
#endif

