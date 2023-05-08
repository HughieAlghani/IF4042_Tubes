#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libretrieve/user_q_cat.c,v 11.0 1992/07/21 18:24:00 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Get query text from a user and put into a temporary file
 *1 retrieve.user_query.cat_skel
 *2 user_q_cat (unused1, textloc, inst)
 *3   char *unused1;
 *3   TEXTLOC *textloc;
 *3   int inst;
 *4 init_user_q_cat (spec, unused)
 *5   "retrieve.query_skel"
 *5   "retrieve.get_query.trace"
 *4 close_user_q_cat (inst)
 *7 Copy the query skeleton given by query_skel to a temporary file
 *7 in /tmp (name will be unique to this instantiation of this procedure.)
 *7 Then take user input and append it to the temp file.  The user input
 *7 is ended by a line containing a single '.'.
 *7 The file location is returned in textloc.
 *7 Return 0 if user submits no query, UNDEF if error, 1 otherwise.
 *9 Functionality mostly replace by user_q_edit
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


static SPEC_PARAM spec_args[] = {
    "retrieve.query_skel",        getspec_dbfile,(char *) &query_skel,
    TRACE_PARAM ("retrieve.get_query.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


static char *query_file = NULL;
static int fd = UNDEF;

int
init_user_q_cat (spec, unused)
SPEC *spec;
char *unused;
{
    char *temp = "/tmp/sm_q_XXXXXX";

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_user_q_cat");

    if (NULL == (query_file = malloc ((unsigned) strlen (temp))))
        return (UNDEF);
    (void) strcpy (query_file, temp);
    (void) mktemp (query_file);


    PRINT_TRACE (2, print_string, "Trace: leaving init_user_q_cat");
    return (0);
}

int
user_q_cat (unused1, textloc, inst)
char *unused1;
TEXTLOC *textloc;
int inst;
{
    SM_BUF newline;
    SM_BUF text;
    char user_input[PATH_LEN];
    int fd_skel;
    long skel_size;                   /* Size of query skel file */

    (void) printf ("Please type a new query, consisting of descriptive natural language words\nand phrases, ending with a line of a single '.'\nAn empty query will exit SMART.\n");
    (void) fflush (stdout);
    text.size = text.end = 0;

    newline.buf = &user_input[0];
    while (NULL != fgets (&user_input[0], PATH_LEN, stdin) &&
           strcmp (&user_input[0], ".\n") != 0) {
        if (user_input[0] != '\n') {
            newline.end = strlen (&user_input[0]);
            if (UNDEF == add_buf (&newline, &text))
                return (UNDEF);
        }
    }

    if (text.end == 0)
        return (0);

    if (-1 == (fd = open (query_file, O_CREAT | O_WRONLY | O_TRUNC, 0600)))
        return (UNDEF);

    if (VALID_FILE (query_skel)) {
        /* Prepend the user query with collection dependent query skeleton */
        if (-1 == (fd_skel = open (query_skel, O_RDONLY)))
            return (UNDEF);
        if (UNDEF == copy_fd (fd_skel, fd))
            return (UNDEF);
        skel_size = lseek (fd_skel, 0L, 2);
        if (UNDEF == close (fd_skel))
            return (UNDEF);
    }
    else
        skel_size = 0;

    if (-1 == write (fd, text.buf, text.end))
        return (UNDEF);
    if (-1 == close (fd))
        return (UNDEF);

    textloc->file_name = query_file;
    textloc->begin_text = 0;
    textloc->end_text = text.end + skel_size;

    PRINT_TRACE (2, print_string, "Trace: leaving user_q_cat");
    return (1);
}

int
close_user_q_cat (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_user_q_cat");
    if (query_file != NULL && fd != UNDEF && -1 == unlink (query_file))
        return (UNDEF);
    query_file = NULL;
    fd = UNDEF;

    PRINT_TRACE (2, print_string, "Trace: leaving close_user_q_cat");
    return (0);
}
