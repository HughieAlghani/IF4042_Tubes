/************************************************************************
*                                                                       *
*       This is a replacement of inter_match.c (by Chris Buckley)       *
*       Instead of the unix-styled interface, we provide x-window       *
*       interface.                                                      *
*       To invoke this procedure, instead of typing in:                 *
*		smart/smart top.inter_match Sample/indexed.adi/spec     *
*	(under directory src.new)					*
*       just type in:							*
*		smart/smart top.x_inter_match Sample/indexed.adi/spec   *
*                                                                       *
*    by Tong Li, graduate student in CS from Oregon State University,   *
*                e-mail: lit@mist.cs.orst.edu                           *
*                                                                       *
*       Victuria Chiu, senior in CS from Univ. of Cal. in Berkeley,     *
*                      e-mail: victoria@csr.lbl.gov                     *
*                                                                       *
*    at Lawrence berkeley Laboratory, summer, 1991                      *
*									*
************************************************************************/

#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/user_contrib/smartx/libtop/x_inter_match.c,v 11.0 1992/07/21 18:19:20 chrisb Exp $";
#endif

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "display.h"
#include "io.h"
#include "smart_error.h"
#include "spec.h"
#include "proc.h"
#include "trace.h"
#include "sm_display.h"
#include "tr_vec.h"
#include "context.h"
#include "retrieve.h"
#include "vector.h"
#include "docindex.h"
#include "inter.h"

/* the header file for x-window interface */
#include "xfunctions.h"

static SPEC_PARAM spec_args[] = {
/*    "inter.verbose",             getspec_long,    (char *) &verbose, */
    TRACE_PARAM ("inter.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);


extern COMMAND *parse_command_line();
extern int help_message(); 

/* Command procedures */
static int quit_inter(), exit_inter();
int show_titles();
int init_show_doc(), show_next_doc(), close_show_doc(), show_named_doc(),
    show_current_doc(), show_query();
int init_show_vec(), show_dvec(), show_qvec(), close_show_vec();
int init_run_vec(), run_qvec(), close_run_vec(), run_new(), x_run_new(),
  run_feedback(), run_dvec();
int inter_save_file();
int toggle_raw_doc();
int show_raw_tr(), init_show_spec(), show_spec(), show_raw_display();
int init_sim_vec(), sim_qvec(), sim_dvec(), close_sim_vec();
int init_match_vec(), match_qvec(), match_dvec(), close_match_vec();
int EMPTY();

/* show keywords, to be developed */

int show_keywords()
{
};

/* These COMMANDs have been modified for our "help window" display. */

static COMMAND standard[] = {
/* the old "run" */
    "retrieve", init_run_vec, x_run_new, close_run_vec,
    1, 1, 0,
    "Run new query.  Retrieve documents after getting a new query. \n\
            Enter natural language query in the text panel then press \n\
            the 'Retrieve' button.",

    "feedback <usefuldoc1> <usefuldoc2> ...",
         init_run_vec, run_feedback, close_run_vec,
    1, 1, 0,
    "Retrieve more documents for this query,\n\
            using original query plus the given list of useful documents. \n\
            Select the list of useful documents in the title list panel, \n\
            then press the 'Feedback' button.",

    "more", init_run_vec, run_qvec, close_run_vec,
    1, 1, 0,
    "Retrieve more documents for this query, using only the original query",

    "next", init_show_doc, show_next_doc, close_show_doc,
    1, 1, 0,
    "Show the next ranked document (default action)",

/* to be developed */
    "keywords", EMPTY, show_keywords, EMPTY,
    1, 1, 0,
    "query documents using keywords.(To be developed)",

    "Clear", EMPTY, EMPTY, EMPTY,
    1, 0, 0,
    "Clear the user query text panel for the next retrieval",

    "<did>", init_show_doc, show_named_doc, close_show_doc,
     7, 1, 0,
    "Double click a title to display the detail description of the document",

    "help", EMPTY, help_message, EMPTY,
    1, 0, 0,
    "Display this help message panel",

    "quit",     EMPTY, quit_inter, EMPTY,
    1, 0, 1,
    "Quit SMART. Any designated actions are done",

    "xit"     , EMPTY, exit_inter, EMPTY,
    1, 0, 1,
    "Exit SMART. No designated actions are done",

    "save", EMPTY, inter_save_file, EMPTY,
    1, 0, 0,
    "Append a copy of the output of the last command to the named file",

    NULL, NULL, NULL, NULL,
    1, 0, 0, ""
};


static COMMAND advanced[] = {
    "Raw_doc", init_show_doc, toggle_raw_doc, close_show_doc,
    1, 1, 0,
    "Toggle to see original raw doc instead of formatted doc",

    "Top_ranked", EMPTY, show_raw_tr, EMPTY,
    1, 1, 0,
    "Display raw retrieval information",
    
    "Location", EMPTY, show_raw_display, EMPTY,
    1, 1, 0,
    "Display raw display information about location of documents",
    
    "Spec_file", init_show_spec, show_spec, EMPTY,
    1, 1, 0,
    "Display full specifications for retrieval/collection",
    
    "Dtext <did>", init_show_doc, show_named_doc, close_show_doc,
    2, 1, 0,
    "Display text of doc <did> (Same as <did>)",
    
    "Dvec <did>",  init_show_vec, show_dvec, close_show_vec,
    2, 1, 0,
    "Display the indexed document vector",

    "Drun <did>",  init_run_vec, run_dvec, close_run_vec,
    2, 1, 0,
    "Run new query, using <did> as the new query",
    
    "Dsim <did1> <did2>", init_sim_vec, sim_dvec, close_sim_vec,
    2, 1, 0,
    "Report similarity of doc 1 to doc 2",
    
    "Dmatch <did1> <did2>", init_match_vec, match_dvec, close_match_vec,
    2, 1, 0,
    "Report matching terms between doc 1 and doc 2",

    "Qtext", EMPTY, show_query, EMPTY,
    2, 1, 0,
    "Display text of last user supplied query",

    "Qvec", init_show_vec, show_qvec, close_show_vec,
    2, 1, 0,
    "Display the current indexed query vector",
    
    "Qsim <did1>", init_sim_vec, sim_qvec, close_sim_vec,
    2, 1, 0,
    "Report similarity of doc 1 to query",

    "Qmatch <did1>", init_match_vec, match_qvec, close_match_vec,
    2, 1, 0,
    "Report matching terms between doc 1 and query",

    NULL, NULL, NULL, NULL,
    1, 0, 0, ""
};

static INTER_STATE is;
static QUERY_VECTOR query_vec;
static TR_VEC tr_vec;


/* this procedure is the same as init_inter_match()  */

int
init_x_inter_match (spec, unused)
SPEC *spec;
char *unused;
{
    COMMAND *command;

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (1, print_string, "Trace: entering init_x_inter_match");

    /* Initialize utility procedures */
    if (UNDEF == init_inter_util (spec, (char *) NULL))
        return (UNDEF);

    /* Initialize all command procedures */
    /* NOTE: ignoring instantiation value returned by init... */
    for (command = &standard[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.init_proc (spec, (char *) NULL))
            return (UNDEF);
    }
    for (command = &advanced[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.init_proc (spec, (char *) NULL))
            return (UNDEF);
    }

    is.output_buf.size = 0;
    is.err_buf.size = 0;
    
    query_vec.vector = (char *) NULL;
    query_vec.qid = -1;
    tr_vec.num_tr = 0;
    is.retrieval.query = &query_vec;
    is.retrieval.tr = &tr_vec;
    is.query_textdisplay.file_name = (char *) NULL;

    PRINT_TRACE (1, print_string, "Trace: leaving init_x_inter_match");
    return (0);
}


int
x_inter_match (unused1, unused2, inst)
char *unused1, *unused2;
int inst;
{
    PRINT_TRACE (1, print_string, "Trace: entering x_inter_match");

    /* This help_message() is to put the explainations in both "standard" and
       "advanced" COMMANDs into is.err_buf.buf. */
    (void) help_message ((INTER_STATE *) NULL, (char *) NULL);

    /* turn control to x-window */

    xinterface(is);

}

/* this procedure is the same as close_inter_match (). */

int
close_x_inter_match (inst)
int inst;
{
    COMMAND *command;

    PRINT_TRACE (1, print_string, "Trace: entering close_x_inter_match");

    /* Close all command procedures */
    for (command = &standard[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.close_proc (0))
            return (UNDEF);
    }
    for (command = &advanced[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.close_proc (0))
            return (UNDEF);
    }

    if (UNDEF == close_inter_util (0))
        return (UNDEF);

    if (is.output_buf.size > 0)
        (void) free (is.output_buf.buf);
    if (is.err_buf.size > 0)
        (void) free (is.err_buf.buf);

    PRINT_TRACE (1, print_string, "Trace: leaving close_x_inter_match");
    return (0);
}


static int
quit_inter ()
{
    /* For now, no intermediate actions (eventually being collection
       maintainance - delete, edit, update user profile, etc); */
    return (exit_inter ());
}

static int
exit_inter ()
{
    return (0);
}

static int
show_help (help_commands)
COMMAND *help_commands;
{
    char temp_buf[PATH_LEN];

    while (help_commands->proc.name) {
        (void) sprintf (temp_buf,
                        "%-10s: %s\n",
                        help_commands->proc.name,
                        help_commands->explanation);
        if (UNDEF == add_buf_string (temp_buf, &is.err_buf))
            return (UNDEF);
        help_commands++;
    }

    return (0);
}

/* This help_message() does both the old "help_message()" and 
   "advanced_help_message()" for the "help window" of our X_Window interface. */

extern int
help_message (is_unused, unused)
INTER_STATE *is_unused;
char *unused;
{
    add_buf_string ("\n Main functions:\n", &is.err_buf);
    show_help (&standard[0]);
    add_buf_string ("\n Advanced functions:\n", &is.err_buf);

    return (show_help (&advanced[0]));
}


COMMAND *
parse_command_line (buf,is_x)
char *buf;
INTER_STATE *is_x;

{
    int len;
    COMMAND *command;

    /* if a number was specified, want to print that doc */
    if (*buf <= '9' && *buf >= '0') {

        is_x->command_line[0] = "Dtext";
        is_x->command_line[1] = buf;
        while (*buf && ! isspace (*buf))
            buf++;
        *buf = '\0';
        is_x->num_command_line = 2;
    }
    else {
        /* Separate command line into tokens */
        is_x->num_command_line = 0;
        while (*buf) {
            /* skip leading white space */
            while (*buf && isspace (*buf))
                buf++;
            if (*buf == '\0' && is_x->num_command_line == 0) {
                /* Default command is to show next doc */
                buf = "next";
            }
            if (*buf == '\0')
                break;
            is_x->command_line[is_x->num_command_line++] = buf;
            while (*buf && !isspace (*buf))
                buf++;
            if (*buf)
                *buf++ = '\0';
            if (is_x->num_command_line >= MAX_COMMAND_LINE)
                break;
        }
    }

    /* Find the desired command */
    len = strlen (is_x->command_line[0]);
    for (command =  &standard[0]; command->proc.name; command++) {
        if (0 == strncmp (is_x->command_line[0],
                          command->proc.name,
                          MAX (len, command->sign_len))) {
            break;
        }
    }
    if (command->proc.name == (char *) NULL) {
        /* Search advanced commands */
        for (command =  &advanced[0]; command->proc.name; command++) {
            if (0 == strncmp (is_x->command_line[0],
                              command->proc.name,
                              MAX (len, command->sign_len)))
                break;
        }
    }
    if (command->proc.name) {
        /* Have the output command name be the full name */
        is_x->command_line[0] = command->proc.name;
        return (command);
    }
    return ((COMMAND *) NULL);
}

