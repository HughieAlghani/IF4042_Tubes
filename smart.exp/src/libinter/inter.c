#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/inter.c,v 11.0 1992/07/21 18:21:43 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "textloc.h"
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

static long verbose;         /* Indicate level of description to user */

static SPEC_PARAM spec_args[] = {
    "inter.verbose",             getspec_long,    (char *) &verbose,
    TRACE_PARAM ("inter.trace")
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

typedef struct {
    PROC_TAB proc;           /* Procedure to do action. All procedures
                                return 1 upon success, UNDEF upon
                                system failure, 0 upon minor failure */
    int sign_len;            /* Number of chars significant in command.name */
    int save_output;         /* If false, do not keep output of command
                                (cannot "save" it to a file) */
    int end_inter;           /* If true, exit inter after this command */
    char *explanation;       /* Description of action taken by name */
} COMMAND;


static COMMAND *parse_command_line();
static int show_output();
static int save_command_name();
static int initialize_is(), close_is();

/* Command procedures */
static int quit_inter(), exit_inter();
static int help_message(), advanced_help_message(), experiment_help_message();
int show_titles();
int init_show_doc(), show_next_doc(), close_show_doc(), show_named_doc(),
    show_current_doc(), show_query();
int init_show_vec(), show_dvec(), show_qvec(), close_show_vec();
int init_show_inv(), show_inv(), close_show_inv();
int init_run_vec(), run_qvec(), close_run_vec(), run_new(), run_feedback(),
    run_dvec();
int inter_save_file();
int toggle_raw_doc();
int show_raw_tr(), init_show_spec(), show_spec(), show_raw_textloc();
int init_sim_vec(), sim_qvec(), sim_dvec(), close_sim_vec();
int init_match_vec(), match_qvec(), match_dvec(), close_match_vec();
int EMPTY();

int init_spec_list(), show_spec_list(), add_spec_list(), use_spec_list();
int init_set_query(), set_query(), close_set_query();
int init_show_exp(), close_show_exp(), show_exp_rr(), show_exp_tr(),
    show_exp_eval(), show_exp_rclprn(), show_exp_tr_eval(), show_exp_rr_eval(),
    show_exp_ind_rr(), show_exp_ind_tr(),
/* OLD */
    show_exp_trprn(), show_exp_trrcl(), show_exp_trtprn(), show_exp_toptr(),
    show_exp_topltr(), show_exp_results();

static COMMAND standard[] = {
    "titles",    EMPTY, show_titles, EMPTY,
    1, 1, 0,
    "Show list of titles retrieved for this query again",

    "next", init_show_doc, show_next_doc, close_show_doc,
    1, 1, 0,
    "Show the next ranked document (default action)",

    "<did>", init_show_doc, show_named_doc, close_show_doc,
     7, 1, 0,
    "Show document <did>",

    ".", init_show_doc, show_current_doc, close_show_doc,
    1, 1, 0,
    "Show the current document again",

    "more", init_run_vec, run_qvec, close_run_vec,
    1, 1, 0,
    "Retrieve more documents for this query, using only original query",

    "run", init_run_vec, run_new, close_run_vec,
    1, 1, 0,
    "Run new query.  Retrieve documents after getting a new query.",

    "feedback <usefuldoc1> <usefuldoc2> ...",
         init_run_vec, run_feedback, close_run_vec,
    1, 1, 0,
    "Retrieve more documents for this query,\n\
            using original query plus the given list of useful documents",

    "quit",     EMPTY, quit_inter, EMPTY,
    1, 0, 1,
    "Quit SMART. Any designated actions are done",

    "xit"     , EMPTY, exit_inter, EMPTY,
    1, 0, 1,
    "Exit SMART. No designated actions are done",

    "save file", EMPTY, inter_save_file, EMPTY,
    1, 0, 0,
    "Append copy of output of last command to the named file",
    
    "?", EMPTY, help_message, EMPTY,
    1, 0, 0,
    "Display this help message",
    
    "help", EMPTY, help_message, EMPTY,
    1, 0, 0,
    "Display this help message",

    "advanced", EMPTY, advanced_help_message, EMPTY,
    1, 0, 0,
    "Display help message describing other advanced options",

    "experimental", EMPTY, experiment_help_message, EMPTY,
    1, 0, 0,
    "Display help message for examining experimental results",

    NULL, NULL, NULL, NULL,
    1, 0, 0, ""
};


static COMMAND advanced[] = {
    "Raw_doc", init_show_doc, toggle_raw_doc, close_show_doc,
    1, 1, 0,
    "Toggle to see original raw doc instead of formatted doc",

    "Inv <con> [<ctype>]", init_show_inv, show_inv, close_show_inv,
    1, 1, 0,
    "Show inverted list for con, ctype",

    "Top_ranked", EMPTY, show_raw_tr, EMPTY,
    1, 1, 0,
    "Display raw retrieval information",
    
    "Location", EMPTY, show_raw_textloc, EMPTY,
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
    
    "Qsim <did>", init_sim_vec, sim_qvec, close_sim_vec,
    2, 1, 0,
    "Report similarity of doc to query",

    "Qmatch <did>", init_match_vec, match_qvec, close_match_vec,
    2, 1, 0,
    "Report matching terms between doc and query",

    NULL, NULL, NULL, NULL,
    1, 0, 0, ""
};

static COMMAND experiment[] = {
    "Elist", init_spec_list, show_spec_list, EMPTY,
    2, 1, 0,
    "Show list of experimental spec files",

    "Eadd_spec spec_file", EMPTY, add_spec_list, EMPTY,
    2, 1, 0,
    "Add to list of experimental spec files",

    "Edel_spec spec_file", EMPTY, EMPTY, EMPTY,
    2, 1, 0,
    "Delete from list of experimental spec files NOT IMPLEM",

    "Euse_spec spec_file", EMPTY, use_spec_list, EMPTY,
    2, 1, 0,
    "Use spec_file as the current spec",

    "Eqid qid", init_set_query, set_query, close_set_query,
    3, 1, 0,
    "Set current query to be qid within current spec file",

    "Err", init_show_exp, show_exp_rr, close_show_exp,
    3, 1, 0,
    "Show ranks of rel docs for current query",

    "Etr", init_show_exp, show_exp_tr, close_show_exp,
    3, 1, 0,
    "Show ranks of top docs for current query",

    "Eq_eval", init_show_exp, show_exp_eval, close_show_exp,
    4, 1, 0,
    "Show eval output for current query",

    "Eeval_rr", EMPTY, show_exp_rr_eval, EMPTY,
    7, 1, 0,
    "Give evaluation results for all spec_list, based on rel docs",

    "Eeval_tr", EMPTY, show_exp_tr_eval, EMPTY,
    7, 1, 0,
    "Give evaluation results for all spec_list, based on top docs",

    "Eind_rr", EMPTY, show_exp_ind_rr, EMPTY,
    6, 1, 0,
    "Give individual query ranks of rel docs for all spec_list",

    "Eind_tr", EMPTY, show_exp_ind_tr, EMPTY,
    6, 1, 0,
    "Give individual query summary of top docs for all spec_list",

    "Ercl_prn", EMPTY, show_exp_rclprn, EMPTY,
    3, 1, 0,
    "Give query by query recall precision for all spec_list\n",

    NULL, NULL, NULL, NULL,
    1, 0, 0, ""
};

static INTER_STATE is;
static QUERY_VECTOR query_vec;
static TR_VEC tr_vec;

int
init_inter (spec, unused)
SPEC *spec;
char *unused;
{
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_inter");

    is.output_buf.size = 0;
    is.err_buf.size = 0;
    is.spec_list.num_spec = 0;
    if (UNDEF == initialize_is (spec, &is))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving init_inter");
    return (0);
}



int
inter (unused1, unused2, inst)
char *unused1, *unused2;
int inst;
{
    char in_buffer[PATH_LEN];   /* current command from user */
    COMMAND *command;
    int status;

    PRINT_TRACE (2, print_string, "Trace: entering inter");

    if (verbose) {
        (void) help_message ((INTER_STATE *) NULL, (char *) NULL);
        (void) show_output (&is.err_buf);
    }

    while (1) {
        if (verbose) {
            (void) fputs ("\nSmart (ntq?): ", stdout);
            (void) fflush (stdout);
        }
        /* Discover what command to execute next */
        if (NULL == fgets (in_buffer, PATH_LEN, stdin)) {
            (void) fputs ("\n", stdout);
            return (0);
        }

        is.err_buf.end = 0;

        if (NULL == (command = parse_command_line (in_buffer))) {
            if (UNDEF == add_buf_string
                ("Unrecognized command. Legal commands are\n\n", &is.err_buf))
                return (UNDEF);
            (void) help_message ((INTER_STATE *) NULL, (char *) NULL);
            (void) show_output (&is.err_buf);
            continue;
        }

        if (command->save_output) {
            if (verbose) {
                /* Save the current command line to be output later */
                if (UNDEF == save_command_name())
                    return (UNDEF);
            }
            else
                is.output_buf.end = 0;
        }

        /* Run the new command.  Output goes to output_buf unless error */
        /* If major error, then exit, else print err_buf */
        if (UNDEF == (status = command->proc.proc (&is, (char *) NULL)) ||
            command->end_inter) {
            if (command->end_inter)
                PRINT_TRACE (2, print_string, "Trace: leaving inter");
            return (status);
        }
        if (command->save_output == 0 || status == 0) 
            (void) show_output (&is.err_buf);
        else
            (void) show_output (&is.output_buf);
    }
}

int
close_inter (inst)
int inst;
{
    PRINT_TRACE (2, print_string, "Trace: entering close_inter");

    if (UNDEF == close_is (&is))
        return (UNDEF);

    if (is.output_buf.size > 0)
        (void) free (is.output_buf.buf);
    if (is.err_buf.size > 0)
        (void) free (is.err_buf.buf);

    PRINT_TRACE (2, print_string, "Trace: leaving close_inter");
    return (0);
}

static int
initialize_is (spec, is)
SPEC *spec;
INTER_STATE *is;
{
    COMMAND *command;

    /* Initialize utility procedures */
    if (UNDEF == init_inter_util (spec, (char *) is))
        return (UNDEF);

    /* Initialize all command procedures */
    /* NOTE: ignoring instantiation value returned by init... */
    for (command = &standard[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.init_proc (spec, (char *) is))
            return (UNDEF);
    }
    for (command = &advanced[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.init_proc (spec, (char *) is))
            return (UNDEF);
    }
    for (command = &experiment[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.init_proc (spec, (char *) is))
            return (UNDEF);
    }

    query_vec.vector = (char *) NULL;
    query_vec.qid = -1;
    tr_vec.num_tr = 0;
    is->retrieval.query = &query_vec;
    is->retrieval.tr = &tr_vec;
    is->query_textloc.file_name = (char *) NULL;
    is->current_spec = spec;
    is->verbose_level = verbose;
    return (1);
}

static int
close_is (is)
INTER_STATE *is;
{
    COMMAND *command;
    /* Close all command procedures */
    for (command = &standard[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.close_proc (0))
            return (UNDEF);
    }
    for (command = &advanced[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.close_proc (0))
            return (UNDEF);
    }
    for (command = &experiment[0]; command->proc.name; command++) {
        if (UNDEF == command->proc.close_proc (0))
            return (UNDEF);
    }

    if (UNDEF == close_inter_util (0))
        return (UNDEF);
    return (1);
}

/* Re-initialize is according to (new) spec file spec */
int
reset_is (spec, is)
SPEC *spec;
INTER_STATE *is;
{
    if (UNDEF == close_is (is) ||
        UNDEF == initialize_is (spec, is))
        return (UNDEF);
    return (1);
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

static int
help_message (is_unused, unused)
INTER_STATE *is_unused;
char *unused;
{
    return (show_help (&standard[0]));
}

static int
advanced_help_message (is_unused, unused)
INTER_STATE *is_unused;
char *unused;
{
    return (show_help (&advanced[0]));
}


static int
experiment_help_message (is_unused, unused)
INTER_STATE *is_unused;
char *unused;
{
    return (show_help (&experiment[0]));
}


static int
show_output (buf)
SM_BUF *buf;
{
    int num_written, newly_written;

    if (verbose) {
        if (UNDEF == unix_page (buf->buf, buf->end))
            return (UNDEF);
    }
    else {
        num_written = 0;
        while (num_written < buf->end) {
            if (-1 == (newly_written = fwrite (buf->buf + num_written,
                                               1,
                                               buf->end - num_written,
                                               stdout)))
                return (UNDEF);
            num_written += newly_written;
        }
    }
    return (1);
}

static COMMAND *
parse_command_line (buf)
char *buf;
{
    int len;
    COMMAND *command;

    /* if a number was specified, want to print that doc */
    if ( *buf=='(' || (*buf <= '9' && *buf >= '0')) {
        is.command_line[0] = "Dtext";
        is.command_line[1] = buf;
        while (*buf && ! isspace (*buf))
            buf++;
        *buf = '\0';
        is.num_command_line = 2;
    }
    else {
        /* Separate command line into tokens */
        is.num_command_line = 0;
        while (*buf) {
            /* skip leading white space */
            while (*buf && isspace (*buf))
                buf++;
            if (*buf == '\0' && is.num_command_line == 0) {
                /* Default command is to show next doc */
                buf = "next";
            }
            if (*buf == '\0')
                break;
            is.command_line[is.num_command_line++] = buf;
            while (*buf && !isspace (*buf))
                buf++;
            if (*buf)
                *buf++ = '\0';
            if (is.num_command_line >= MAX_COMMAND_LINE)
                break;
        }
    }

    /* Find the desired command */
    len = strlen (is.command_line[0]);
    for (command =  &standard[0]; command->proc.name; command++) {
        if (0 == strncmp (is.command_line[0],
                          command->proc.name,
                          MAX (len, command->sign_len))) {
            break;
        }
    }
    if (command->proc.name == (char *) NULL) {
        /* Search advanced commands */
        for (command =  &advanced[0]; command->proc.name; command++) {
            if (0 == strncmp (is.command_line[0],
                              command->proc.name,
                              MAX (len, command->sign_len)))
                break;
        }
    }
    if (command->proc.name == (char *) NULL) {
        /* Search experiment commands */
        for (command =  &experiment[0]; command->proc.name; command++) {
            if (0 == strncmp (is.command_line[0],
                              command->proc.name,
                              MAX (len, command->sign_len)))
                break;
        }
    }
    if (command->proc.name) {
        /* Have the output command name be the full name */
        is.command_line[0] = command->proc.name;
        return (command);
    }
    return ((COMMAND *) NULL);
}

/* Save the current command name and arguments in is.output_buf */
static int
save_command_name ()
{
    char temp_buf[PATH_LEN];
    char *ptr, *buf_ptr;
    long i;

    is.output_buf.end = 0;
    buf_ptr = temp_buf;
    for (ptr = "\n-------------------------\n"; *ptr;)
        *buf_ptr++ = *ptr++;
    for (ptr = is.command_line[0]; *ptr && !(isspace (*ptr));)
        *buf_ptr++ = *ptr++;
    *buf_ptr++ = ' ';
    for (i = 1; i < is.num_command_line; i++) {
        for (ptr = is.command_line[i]; *ptr;)
            *buf_ptr++ = *ptr++;
        *buf_ptr++ = ' ';
    }
    for (ptr = "\n----\n"; *ptr;)
        *buf_ptr++ = *ptr++;
    *buf_ptr++ = '\0';
    if (UNDEF == add_buf_string (temp_buf, &is.output_buf))
        return (UNDEF);
    return (1);
}
