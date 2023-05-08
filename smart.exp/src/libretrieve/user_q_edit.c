#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/user_q_edit.c,v 11.0 1992/07/21 18:24:01 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Get query text from a user (possibly via editor) and put into a temp file
 *1 retrieve.user_query.edit_skel
 *2 user_q_edit (unused1, textloc, inst)
 *3   char *unused1;
 *3   TEXTLOC *textloc;
 *3   int inst;
 *4 init_user_q_edit (spec, unused)
 *5   "retrieve.query_skel"
 *5   "retrieve.verbose"
 *5   "retrieve.get_query.editor_only"
 *5   "retrieve.get_query.trace"
 *4 close_user_q_edit (inst)
 *7 Copy the query skeleton given by query_skel to a temporary file
 *7 in /tmp (name will be unique to this instantiation of this procedure.)
 *7 Then take user input and append it to the temp file.  The user input
 *7 is ended by a line containing a single '.'.  If the user types "~e",
 *7 then an editor is called for the temporary file constructed so far.
 *7 If verbose is zero, then the normal prompts to the user are not given
 *7 (this makes it more suitable for shell scripts and the like).
 *7 The editor_only boolean variable indicates whether the editor should
 *7 be invoked for the entire query acquisition.
 *7 The file location is returned in textloc.
 *7 Return 0 if user submits no query, UNDEF if error, 1 otherwise.
 *9 Should be a flag to put user in editor initially?
***********************************************************************/

#include <fcntl.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "trace.h"
#include "context.h"
#include "retrieve.h"
#include "vector.h"
#include "docindex.h"
#include "textloc.h"
#include "buf.h"

/* Get the next query from the user, then index it and return it */

static char *query_skel;          /* Skeleton query for user to fill in */
static long verbose_level;        /* Level of printing to be shown user
                                     1 == normal, 0 == none, > 1 demo */
static long editor_only;          /* Boolean flag to indicate if editor to
                                     be used exclusively to get query */
static SPEC_PARAM spec_args[] = {
    "retrieve.query_skel",        getspec_dbfile, (char *) &query_skel,
    "retrieve.verbose",           getspec_long,   (char *) &verbose_level,
    "retrieve.get_query.editor_only", getspec_bool, (char *) &editor_only,
    TRACE_PARAM ("retrieve.get_query.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


static char *query_file = NULL;
static int fd = UNDEF;

static int edit_query();

int
init_user_q_edit (spec, unused)
SPEC *spec;
char *unused;
{
    char *temp = "/tmp/sm_q_XXXXXX";

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_user_q_edit");

    if (NULL == (query_file = malloc ((unsigned) strlen (temp))))
        return (UNDEF);
    (void) strcpy (query_file, temp);
    (void) mktemp (query_file);


    PRINT_TRACE (2, print_string, "Trace: leaving init_user_q_edit");
    return (0);
}

int
user_q_edit (unused1, textloc, inst)
char *unused1;
TEXTLOC *textloc;
int inst;
{
    SM_BUF text;
    char user_input[PATH_LEN];
    int fd_skel;
    int editted_flag;

    if (-1 == (fd = open (query_file, O_CREAT | O_WRONLY | O_TRUNC, 0600)))
        return (UNDEF);

    if (VALID_FILE (query_skel)) {
        /* Prepend the user query with collection dependent query skeleton */
        if (-1 == (fd_skel = open (query_skel, O_RDONLY)))
            return (UNDEF);
        if (UNDEF == copy_fd (fd_skel, fd))
            return (UNDEF);
        if (UNDEF == close (fd_skel))
            return (UNDEF);
    }
    text.size = text.end = 0;
    editted_flag = 0;

    if (editor_only) {
        if (UNDEF == edit_query (fd, &text))
            return (UNDEF);
        editted_flag = 1;
    }
    else {
        if (verbose_level) {
            (void) printf ("Please type a new query, consisting of descriptive natural language words\nand phrases, ending with a line of a single '.'\nAn empty query will exit SMART.\n");
            (void) fflush (stdout);
        }

        while (NULL != fgets (&user_input[0], PATH_LEN, stdin) &&
               strcmp (&user_input[0], ".\n") != 0) {
            if (user_input[0] == '\n')
                continue;
            
            if (user_input[0] == '~') {
                switch (user_input[1]) {
                  case 'e':
                  case 'v':
                    if (UNDEF == edit_query (fd, &text))
                        return (UNDEF);
                    editted_flag = 1;
                    if (verbose_level)
                        (void) printf ("\n(Continue)\n");
                    break;
                  default:
                    if (verbose_level)
                        (void) printf ("~ command not implemented, ignored\n");
                    break;
                }
            }
            else {
                if (UNDEF == add_buf_string (user_input, &text))
                    return (UNDEF);
            }
        }
    }

    if (text.end == 0 && editted_flag == 0)
        return (0);

    if (text.end && text.end != write (fd, text.buf, text.end))
        return (UNDEF);

    textloc->end_text = lseek (fd, 0L, 2);
    textloc->file_name = query_file;
    textloc->begin_text = 0;

    if (-1 == close (fd))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving user_q_edit");
    return (1);
}

int
close_user_q_edit (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_user_q_edit");
    if (query_file != NULL && fd != UNDEF && -1 == unlink (query_file))
        return (UNDEF);
    query_file = NULL;
    fd = UNDEF;

    PRINT_TRACE (2, print_string, "Trace: leaving close_user_q_edit");
    return (0);
}

static int
edit_query (fd, q_buf)
int fd;
SM_BUF *q_buf;
{
    /* Write out any partially completed query */
    if (q_buf->end != 0) {
        if (q_buf->end != write (fd, q_buf->buf, q_buf->end))
            return (UNDEF);
        q_buf->end = 0;
    }

    /* Invoke editor on query */
    if (UNDEF == unix_edit_file (query_file))
        return (UNDEF);

    (void) lseek (fd, 0L, 2);

    return (1);
}
