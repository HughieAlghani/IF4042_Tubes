#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/user_contrib/smartx/libinter/get_q_user.c,v 11.0 1992/07/21 18:19:19 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/************************************************************************
*                                                                       *
*       For the X interface, x_get_q_user() was added.                  *
*                                                                       *
*    by Tong Li, graduate student in CS from Oregon State University,   *
*                e-mail: lit@mist.cs.orst.edu                           *
*                                                                       *
*       Victuria Chiu, senior in CS from Univ. of Cal. at Berkeley,     *
*                      e-mail: victoria@csr.lbl.gov                     *
*                                                                       *
*    at Lawrence berkeley Laboratory, summer 1991                       *
*                                                                       *
************************************************************************/

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
#include "sm_display.h"

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
init_get_q_user (spec, unused)
SPEC *spec;
char *unused;
{
    char *temp = "/tmp/sm_q_XXXXXX";

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (1, print_string, "Trace: entering init_get_q_user");

    if (NULL == (query_file = malloc ((unsigned) strlen (temp))))
        return (UNDEF);
    strcpy (query_file, temp);
    (void) mktemp (query_file);


    PRINT_TRACE (1, print_string, "Trace: leaving init_get_q_user");
    return (0);
}

int
get_q_user (unused1, display, inst)
char *unused1;
SM_TEXTDISPLAY *display;
int inst;
{
    SM_BUF newline;
    SM_BUF text;
    char user_input[PATH_LEN];
    int fd_skel;
    long skel_size;                   /* Size of query skel file */

    printf ("Please type a new query, consisting of descriptive natural language words\nand phrases, ending with a line of a single '.'\nAn empty query will exit SMART.\n");
    fflush (stdout);
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
        /* Prepend the user query with collection dependant query skeleton */
        if (-1 == (fd_skel = open (query_skel, O_RDONLY)))
            return (UNDEF);
        if (UNDEF == copy_fd (fd_skel, fd))
            return (UNDEF);
        skel_size = lseek (fd_skel, 0L, 2);
        if (UNDEF == close (fd_skel))
            return (UNDEF);
    }
    if (-1 == write (fd, text.buf, text.end))
        return (UNDEF);
    if (-1 == close (fd))
        return (UNDEF);

    display->file_name = query_file;
    display->begin_text = 0;
    display->end_text = text.end + skel_size;

    PRINT_TRACE (1, print_string, "Trace: leaving get_q_user");
    return (1);
}

/*modified from get_q_user*/

int
x_get_q_user (userinput, display, inst)
char *userinput;
SM_TEXTDISPLAY *display;
int inst;
{
    SM_BUF newline;
    SM_BUF text;
    int fd_skel;
    long skel_size;                   /* Size of query skel file */

    text.size = text.end = 0;

    newline.buf = userinput;
    newline.end = strlen (userinput);
    if (UNDEF == add_buf (&newline, &text))
      return (UNDEF);

    if (text.end == 0)
        return (0);

    if (-1 == (fd = open (query_file, O_CREAT | O_WRONLY | O_TRUNC, 0600)))
        return (UNDEF);
    if (VALID_FILE (query_skel)) {
        /* Prepend the user query with collection dependant query skeleton */
        if (-1 == (fd_skel = open (query_skel, O_RDONLY)))
            return (UNDEF);
        if (UNDEF == copy_fd (fd_skel, fd))
            return (UNDEF);
        skel_size = lseek (fd_skel, 0L, 2);
        if (UNDEF == close (fd_skel))
            return (UNDEF);
    }
    if (-1 == write (fd, text.buf, text.end))
        return (UNDEF);
    if (-1 == close (fd))
        return (UNDEF);

    display->file_name = query_file;
    display->begin_text = 0;
    display->end_text = text.end + skel_size;

    PRINT_TRACE (1, print_string, "Trace: leaving get_q_user");
    return (1);
}

int
close_get_q_user (inst)
int inst;
{
    PRINT_TRACE (1, print_string, "Trace: entering close_get_q_user");
    if (query_file != NULL && fd != UNDEF && -1 == unlink (query_file))
        return (UNDEF);
    query_file = NULL;
    fd = UNDEF;

    PRINT_TRACE (1, print_string, "Trace: leaving close_get_q_user");
    return (0);
}
