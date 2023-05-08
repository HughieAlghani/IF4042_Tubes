#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/smart/smart.c,v 11.0 1992/07/21 18:19:10 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "io.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "accounting.h"

static char *usage = "Usage: smart action spec [spec_param spec_value]*";

static PROC_INST top_level_action;

static SPEC_PARAM top_level_arg = {
    "",                      getspec_func,     (char *) &top_level_action.ptab
    };

static char *trace_file;

static SPEC_PARAM spec_args[] = {
    "global_trace_start", getspec_long, (char *) &global_trace_start,
    "global_trace_end", getspec_long, (char *) &global_trace_end,
    "global_start",     getspec_long, (char *) &global_start,
    "global_end",       getspec_long, (char *) &global_end,
    "global_accounting",getspec_long, (char *) &global_accounting,
    "global_trace_file",getspec_dbfile, (char *) &trace_file,
    TRACE_PARAM ("smart.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

/* Declaration of the root of the procedure hierarchy. Here so the
   entire hierarchy will be loaded from libraries. */
extern TAB_PROC_TAB proc_root[];
extern int num_proc_root;
TAB_PROC_TAB proc_root_root = {
    "root",     TPT_TAB,        proc_root,      NULL,   &num_proc_root
    };

void
main (argc, argv)
int argc;
char *argv[];
{
    SPEC spec;
    char *spec_file;

#ifdef DEBUG2
    malloc_debug(2);
#endif

    if (argc < 3) {
        fprintf (stderr, "%s\n", usage);
        exit (1);
    }

    /* First action gives top level procedure to call */
    top_level_arg.param = argv[1];
    
    argc--;
    argv++;

    spec_file = argv[1];
    /* Read the specification file given by the second argument */
    if (UNDEF == get_spec (spec_file, &spec)) {
        print_error ("smart: Illegal spec file", "Quit");
        exit (2);
    }

    /* Add any local modifications specified on the command line */
    if (argc > 2) {
        if (UNDEF == mod_spec (&spec, argc - 2, &argv[2])) {
            print_error ("smart: Command-line incorrect", "Quit");
            exit (2);
        }
    }

    /* Lookup the values of the top level action */
    /* If not a parameter, assume it is an actual function name */
    if (UNDEF == lookup_spec (&spec, &top_level_arg, 1)) {
        clr_err();
        if (UNDEF == getspec_func (&spec,
                                   (char *) NULL,
                                   top_level_arg.param,
                                   &top_level_action.ptab)) {
            print_error ("smart: Top-level Function not found",
                         top_level_arg.param);
            exit (3);
        }
    }

    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (&spec, &spec_args[0], num_spec_args)) {
        print_error ("smart: Global parameters not found", "Quit");
        exit (3);
    }

    /* allow tracing */
    SET_TRACE (0);
#ifdef TRACE
    if (VALID_FILE (trace_file)) {
        if (NULL == (global_trace_fd = fopen (trace_file, "w"))) {
            print_error ("smart: Trace file not accessible", "Quit");
            exit (3);
        }
    }
#endif
    

    PRINT_TRACE (2, print_string, "Trace: entering smart");
    PRINT_TRACE (4, print_spec, &spec);
    
    /* initialize, run and close the top_level procedure specified */
    if (UNDEF == (top_level_action.inst =
                  top_level_action.ptab->init_proc (&spec, NULL))) {
        print_error ("smart Initialization:", "Quit");
        exit (4);
    }
    if (UNDEF == top_level_action.ptab->proc (NULL,
                                              NULL,
                                              top_level_action.inst)) {
        print_error ("smart:", "Quit");
        exit (5);
    }
    if (UNDEF == top_level_action.ptab->close_proc (top_level_action.inst)) {
        print_error ("smart: Close:", "Quit");
        exit (6);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving smart");
#ifdef TRACE
    if (VALID_FILE (trace_file))
        (void) fclose (global_trace_fd);
#endif
    exit (0);
}
